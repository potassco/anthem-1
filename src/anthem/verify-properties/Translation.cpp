#include <anthem/verify-properties/Translation.h>

#include <clingo.hh>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>
#include <anthem/translation-common/ChooseValueInTerm.h>
#include <anthem/translation-common/Input.h>
#include <anthem/translation-common/Output.h>
#include <anthem/translation-common/StatementVisitor.h>
#include <anthem/verify-properties/Body.h>
#include <anthem/verify-properties/Head.h>
#include <anthem/verify-properties/ReplaceConstants.h>
#include <anthem/verify-properties/TranslationContext.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

const auto makeExistentiallyClosedFormula =
	[](auto &&scopedFormula) -> ast::Formula
	{
		if (scopedFormula.freeVariables.empty())
			return std::move(scopedFormula.formula);

		return ast::Exists(std::move(scopedFormula.freeVariables), std::move(scopedFormula.formula));
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto makeUniversallyClosedFormula =
	[](auto &&scopedFormula) -> ast::Formula
	{
		if (scopedFormula.freeVariables.empty())
			return std::move(scopedFormula.formula);

		return ast::ForAll(std::move(scopedFormula.freeVariables), std::move(scopedFormula.formula));
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto declarePredicateParameters =
	[](const auto &predicateDeclaration)
	{
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(predicateDeclaration.arity());

		for (auto i = 0; i < static_cast<int>(predicateDeclaration.arity()); i++)
		{
			parameters.emplace_back(
				std::make_unique<ast::VariableDeclaration>( ast::VariableDeclaration::Type::Head));
			// TODO: should be Domain::Program
			parameters.back()->domain = Domain::Unknown;
		}

		return parameters;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline void read(const Clingo::AST::Rule &rule, Context &context, TranslationContext &translationContext)
{
	ast::VariableDeclarationPointers freeVariables;
	ast::VariableStack variableStack;
	variableStack.push(&freeVariables);

	// Translate the body literals into a conjunction
	const auto translateBody =
		[&]()
		{
			ast::And translatedBody;

			// Translate body literals
			for (const auto &bodyLiteral : rule.body)
			{
				auto argument = bodyLiteral.data.accept(BodyBodyLiteralVisitor(), bodyLiteral, context, freeVariables, variableStack);
				translatedBody.arguments.emplace_back(std::move(argument));
			}

			return translatedBody;
		};

	// Analyze the type of the head of the rule
	const auto headTranslationResult = rule.head.data.accept(HeadLiteralVisitor(), rule.head, context);

	// Translate the head terms
	const auto translateHeadTerms =
		[&](const auto &headParameters, const auto &headArguments, ast::And &formula)
		{
			for (auto i = 0; i < static_cast<int>(headParameters.size()); i++)
			{
				auto &headParameter = *headParameters[i];
				auto &headArgument = headArguments[i];

				auto translatedHeadTerm = translationCommon::chooseValueInTerm(
					headArgument, headParameter, context, freeVariables, variableStack);
				formula.arguments.emplace_back(std::move(translatedHeadTerm));
			}
		};

	switch (headTranslationResult.headType)
	{
		// Translate rules with a single atom in the head
		case HeadType::SingleAtom:
		{
			assert(headTranslationResult.headAtom);
			const auto &headAtom = *headTranslationResult.headAtom;

			// If there are no definitions for this predicate symbol yet, create an empty data structure for it
			if (translationContext.definitions.find(headAtom.predicateDeclaration)
				== translationContext.definitions.cend())
			{
				TranslationContext::Definitions definitions
				{
					declarePredicateParameters(*headAtom.predicateDeclaration),
					std::vector<ast::ScopedFormula>(),
				};

				translationContext.definitions.insert(
					std::make_pair(headAtom.predicateDeclaration, std::move(definitions)));
			}

			auto &definitions = translationContext.definitions[headAtom.predicateDeclaration];

			variableStack.push(&definitions.headAtomParameters);

			auto formula = translateBody();
			translateHeadTerms(definitions.headAtomParameters, headAtom.arguments, formula);

			variableStack.pop();

			// Replace constants with variables
			ast::Formula constantFreeFormula = ast::Formula(std::move(formula));
			replaceConstants(constantFreeFormula, translationContext);

			auto definition = ast::ScopedFormula{std::move(constantFreeFormula), std::move(freeVariables)};
			definitions.definitions.emplace_back(std::move(definition));

			return;
		}
		// Translate simple choice rules
		case HeadType::ChoiceSingleAtom:
			throw LogicException("choice rules with single atoms not supported yet");
		// Translate facts
		case HeadType::Fact:
			throw LogicException("facts not supported yet");
		// Translate integrity constraints
		case HeadType::IntegrityConstraint:
		{
			auto not_ = ast::Not{translateBody()};
			auto scopedFormula = ast::ScopedFormula{std::move(not_), std::move(freeVariables)};
			auto integrityConstraint = makeUniversallyClosedFormula(std::move(scopedFormula));
			translationContext.integrityConstraints.emplace_back(std::move(integrityConstraint));

			return;
		}
	}

	throw LogicException("unreachable code, please report to bug tracker");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(Context &context, TranslationContext &translationContext)
{
	output::PrintContext printContext(context);
	auto &stream = context.logger.outputStream();

	/*// Print translated formulas
	for (auto &definitions : translationContext.definitions)
	{
		stream << "% definitions for " << definitions.first->name << "/" << definitions.first->arity() << std::endl;

		for (auto &definition : definitions.second)
		{
			stream << "%% free variables: ";

			for (auto &freeVariable : definition.freeVariables)
			{
				output::print<output::FormatterHumanReadable>(stream, *freeVariable, printContext, true);
				stream << ", ";
			}

			if (definition.freeVariables.empty())
				stream << "(none)";

			stream << std::endl;

			translationCommon::printFormula(definition.formula, translationCommon::FormulaType::Axiom, context, printContext);
			stream << std::endl;
		}

		stream << std::endl;
	}*/

	std::sort(context.predicateDeclarations.begin(), context.predicateDeclarations.end(),
		[](const auto &x, const auto &y)
		{
			if (x->name != y->name)
				return x->name < y->name;

			return x->arity() < y->arity();
		});

	const auto makeCompletedDefinition =
		[&](auto &predicateDeclaration)
		{
			const auto matchingDefinitions = translationContext.definitions.find(&predicateDeclaration);

			// If the predicate symbol has no definition, build the universally closed negation
			if (matchingDefinitions == translationContext.definitions.cend())
			{
				auto headAtomParameters = declarePredicateParameters(predicateDeclaration);

				ast::Predicate predicate(&predicateDeclaration);

				for (auto i = 0; i < static_cast<int>(headAtomParameters.size()); i++)
					predicate.arguments.emplace_back(ast::Variable(headAtomParameters[i].get()));

				ast::Not not_(std::move(predicate));

				return ast::ScopedFormula(std::move(not_), std::move(headAtomParameters));
			}

			auto &definitions = *matchingDefinitions;

			ast::Or or_;
			or_.arguments.reserve(definitions.second.headAtomParameters.size());

			// Otherwise, build the disjunction of all existentially closed definitions
			for (auto &&definition : definitions.second.definitions)
			{
				auto existentiallyClosedDefinition = makeExistentiallyClosedFormula(std::move(definition));
				or_.arguments.emplace_back(std::move(existentiallyClosedDefinition));
			}

			ast::Predicate predicate(&predicateDeclaration);

			for (auto i = 0; i < static_cast<int>(definitions.second.headAtomParameters.size()); i++)
				predicate.arguments.emplace_back(ast::Variable(definitions.second.headAtomParameters[i].get()));

			ast::Biconditional biconditional{std::move(predicate), std::move(or_)};

			return ast::ScopedFormula(std::move(biconditional), std::move(definitions.second.headAtomParameters));
		};

	if (!translationContext.inputParameters.empty())
	{
		stream << "% input parameters: ";

		for (const auto &constantReplacement : translationContext.constantReplacements)
		{
			if (&constantReplacement.first != &translationContext.constantReplacements.begin()->first)
				stream << ", ";

			output::print<output::FormatterHumanReadable>(stream, *constantReplacement.second, printContext);

			stream << " for ";

			output::print<output::FormatterHumanReadable>(stream, *constantReplacement.first, printContext);
		}

		stream << std::endl;
	}

	for (auto &predicateDeclaration : context.predicateDeclarations)
	{
		stream
			<< "% completed definition for "
			<< predicateDeclaration->name
			<< "/" << predicateDeclaration->arity()
			<< std::endl;

		auto completedDefinition = makeUniversallyClosedFormula(
			makeCompletedDefinition(*predicateDeclaration));

		translationCommon::printFormula(completedDefinition, translationCommon::FormulaType::Axiom,
			context, printContext);

		stream << std::endl;
	}

	if (!translationContext.integrityConstraints.empty())
		stream << "% integrity constraints" << std::endl;

	// Print integrity constraints
	for (auto &integrityConstraint : translationContext.integrityConstraints)
	{
		translationCommon::printFormula(integrityConstraint, translationCommon::FormulaType::Axiom, context, printContext);

		stream << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames, Context &context)
{
	if (fileNames.size() > 1)
		throw TranslationException("only one file may me translated at a time when verifying properties");

	TranslationContext translationContext;

	const auto read =
		[&](const auto &rule)
		{
			verifyProperties::read(rule, context, translationContext);
		};

	const auto readStatement =
		[&](auto &&statement) -> void
		{
			statement.data.accept(StatementVisitor<decltype(read), TranslationContext>(),
				statement, read, context, translationContext);
		};

	translationCommon::readSingleFile(readStatement, fileNames.front(), context);

	translate(context, translationContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: avoid code duplication
void translate(const char *fileName, std::istream &stream, Context &context)
{
	TranslationContext translationContext;

	const auto read =
		[&](const Clingo::AST::Rule &rule)
		{
			// TODO: remove unnecessary namespace specifier
			verifyProperties::read(rule, context, translationContext);
		};

	const auto readStatement =
		[&](const Clingo::AST::Statement &statement)
		{
			statement.data.accept(StatementVisitor<decltype(read), TranslationContext>(), statement,
				read, context, translationContext);
		};

	translationCommon::readSingleStream(readStatement, fileName, stream, context);

	translate(context, translationContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
