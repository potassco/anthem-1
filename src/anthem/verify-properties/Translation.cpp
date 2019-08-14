#include <anthem/verify-properties/Translation.h>

#include <clingo.hh>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/RuleContext.h>
#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>
#include <anthem/translation-common/ChooseValueInTerm.h>
#include <anthem/translation-common/Input.h>
#include <anthem/translation-common/Output.h>
#include <anthem/translation-common/StatementVisitor.h>
#include <anthem/verify-properties/Body.h>
#include <anthem/verify-properties/Head.h>
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

inline void read(const Clingo::AST::Rule &rule, Context &context, TranslationContext &translationContext)
{
	RuleContext ruleContext;
	ast::VariableStack variableStack;
	variableStack.push(&ruleContext.freeVariables);

	ast::And translatedBody;

	// Translate body literals
	for (const auto &bodyLiteral : rule.body)
	{
		auto argument = bodyLiteral.data.accept(BodyBodyLiteralVisitor(), bodyLiteral, context, ruleContext, variableStack);
		translatedBody.arguments.emplace_back(std::move(argument));
	}

	const auto headTranslationResult = rule.head.data.accept(HeadLiteralVisitor(), rule.head, context);

	const auto translateHeadTerms =
		[&](const HeadAtom &headAtom, ast::And &formula)
		{
			for (const auto &argument : headAtom.arguments)
			{
				auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head);

				auto translatedHeadTerm = translationCommon::chooseValueInTerm(argument, *variableDeclaration, context, ruleContext, variableStack);
				formula.arguments.emplace_back(std::move(translatedHeadTerm));

				ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));
			}
		};

	switch (headTranslationResult.headType)
	{
		case HeadType::SingleAtom:
		{
			assert(headTranslationResult.headAtom);
			const auto &headAtom = *headTranslationResult.headAtom;

			auto formula = std::move(translatedBody);
			translateHeadTerms(headAtom, formula);

			ast::ScopedFormula scopedFormula{std::move(formula), std::move(ruleContext.freeVariables)};

			translationContext.definitions[headAtom.predicate.declaration].emplace_back(std::move(scopedFormula));
		}
		default:
			// TODO: implement
			return;
	}

	throw LogicException("unreachable code, please report to bug tracker");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(Context &context, TranslationContext &translationContext)
{
	output::PrintContext printContext(context);
	auto &stream = context.logger.outputStream();

	// Print translated formulas
	for (auto &definitions : translationContext.definitions)
	{
		stream << "# definitions for " << definitions.first->name << "/" << definitions.first->arity() << std::endl;

		for (auto &definition : definitions.second)
		{
			stream << "## free variables: ";

			for (auto &freeVariable : definition.freeVariables)
			{
				output::print<output::FormatterHumanReadable>(stream, *freeVariable, printContext, true);
				stream << ", ";
			}

			stream << std::endl;

			translationCommon::printFormula(definition.formula, translationCommon::FormulaType::Axiom, context, printContext);
			stream << std::endl;
		}

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
