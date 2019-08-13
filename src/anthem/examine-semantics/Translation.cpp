#include <anthem/examine-semantics/Translation.h>

#include <anthem/AST.h>
#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/RuleContext.h>
#include <anthem/examine-semantics/Body.h>
#include <anthem/examine-semantics/Completion.h>
#include <anthem/examine-semantics/Head.h>
#include <anthem/examine-semantics/Simplification.h>
#include <anthem/examine-semantics/IntegerVariableDetection.h>
#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>
#include <anthem/translation-common/Input.h>
#include <anthem/translation-common/Output.h>
#include <anthem/translation-common/Rule.h>
#include <anthem/translation-common/StatementVisitor.h>

namespace anthem
{
namespace examineSemantics
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void read(const Clingo::AST::Rule &rule, Context &context,
	std::vector<ast::ScopedFormula> &scopedFormulas)
{
	RuleContext ruleContext;
	ast::VariableStack variableStack;
	variableStack.push(&ruleContext.freeVariables);

	ast::And antecedent;
	std::optional<ast::Formula> consequent;

	// Collect all head terms
	rule.head.data.accept(HeadLiteralCollectFunctionTermsVisitor(), rule.head, ruleContext);

	// Create new variable declarations for the head terms
	ruleContext.headVariablesStartIndex = ruleContext.freeVariables.size();
	ruleContext.freeVariables.reserve(ruleContext.headTerms.size());

	for (size_t i = 0; i < ruleContext.headTerms.size(); i++)
	{
		auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head);
		ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));
	}

	// Compute consequent
	auto headVariableIndex = ruleContext.headVariablesStartIndex;
	consequent = rule.head.data.accept(examineSemantics::HeadLiteralTranslateToConsequentVisitor(), rule.head, ruleContext, context, headVariableIndex);

	assert(ruleContext.headTerms.size() == headVariableIndex - ruleContext.headVariablesStartIndex);

	if (!consequent)
		throw TranslationException(rule.head.location, "could not translate formula consequent");

	// Generate auxiliary variables replacing the head atom’s arguments
	for (auto i = ruleContext.headTerms.cbegin(); i != ruleContext.headTerms.cend(); i++)
	{
		const auto &headTerm = **i;

		const auto auxiliaryHeadVariableID = ruleContext.headVariablesStartIndex + i - ruleContext.headTerms.cbegin();
		auto element = ast::Variable(ruleContext.freeVariables[auxiliaryHeadVariableID].get());
		auto set = translationCommon::translate(headTerm, ruleContext, context, variableStack);
		auto in = ast::In(std::move(element), std::move(set));

		antecedent.arguments.emplace_back(std::move(in));
	}

	// Translate body literals
	for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
	{
		const auto &bodyLiteral = *i;

		auto argument = bodyLiteral.data.accept(examineSemantics::BodyBodyLiteralTranslateVisitor(), bodyLiteral, ruleContext, context, variableStack);

		if (!argument)
			throw TranslationException(bodyLiteral.location, "could not translate body literal");

		antecedent.arguments.emplace_back(std::move(argument.value()));
	}

	if (!ruleContext.isChoiceRule)
	{
		ast::Implies formula(std::move(antecedent), std::move(consequent.value()));
		ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
		scopedFormulas.emplace_back(std::move(scopedFormula));
		translationCommon::normalizeAntecedent(scopedFormulas.back().formula.get<ast::Implies>());
	}
	else
	{
		const auto createFormula =
			[&](ast::Formula &argument, bool isLastOne)
			{
				auto &consequent = argument;

				if (!isLastOne)
				{
					ast::Implies formula(ast::prepareCopy(antecedent), std::move(consequent));
					ast::ScopedFormula scopedFormula(std::move(formula), {});
					ast::fixDanglingVariables(scopedFormula);
					scopedFormulas.emplace_back(std::move(scopedFormula));
				}
				else
				{
					ast::Implies formula(std::move(antecedent), std::move(consequent));
					ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
					scopedFormulas.emplace_back(std::move(scopedFormula));
				}

				auto &implies = scopedFormulas.back().formula.get<ast::Implies>();
				auto &antecedent = implies.antecedent.get<ast::And>();
				antecedent.arguments.emplace_back(ast::prepareCopy(implies.consequent));
				ast::fixDanglingVariables(scopedFormulas.back());

				translationCommon::normalizeAntecedent(implies);
			};

		if (consequent.value().is<ast::Or>())
		{
			auto &disjunction = consequent.value().get<ast::Or>();

			for (auto &argument : disjunction.arguments)
				createFormula(argument, &argument == &disjunction.arguments.back());
		}
		// TODO: check whether this is really correct for all possible consequent types
		else
			createFormula(consequent.value(), true);
	}
}

void translate(Context &context, std::vector<ast::ScopedFormula> &scopedFormulas)
{
	assert(context.semantics == Semantics::ClassicalLogic);

	output::PrintContext printContext(context);

	const auto performSimplification = (context.performSimplification && context.semantics == Semantics::ClassicalLogic);

	if (!context.performCompletion)
	{
		// Simplify output if specified
		if (performSimplification)
			for (auto &scopedFormula : scopedFormulas)
				examineSemantics::simplify(scopedFormula.formula);

		if (context.showStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#show statements are ignored because completion is not enabled";

		if (context.externalStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#external statements are ignored because completion is not enabled";

		for (const auto &scopedFormula : scopedFormulas)
		{
			translationCommon::printFormula(scopedFormula.formula, translationCommon::FormulaType::Axiom, context, printContext);
			context.logger.outputStream() << std::endl;
		}

		return;
	}

	// Perform completion
	auto completedFormulas = examineSemantics::complete(std::move(scopedFormulas), context);

	for (const auto &predicateDeclaration : context.predicateDeclarations)
	{
		if (predicateDeclaration->isUsed)
			continue;

		// Check for #show statements with undeclared predicates
		if (predicateDeclaration->visibility != ast::PredicateDeclaration::Visibility::Default)
			context.logger.log(output::Priority::Warning)
				<< "#show declaration of “"
				<< predicateDeclaration->name
				<< "/"
				<< predicateDeclaration->arity()
				<< "” does not match any declared predicate";

		// Check for #external statements with undeclared predicates
		if (predicateDeclaration->isExternal && !predicateDeclaration->isUsed)
			context.logger.log(output::Priority::Warning)
				<< "#external declaration of “"
				<< predicateDeclaration->name
				<< "/"
				<< predicateDeclaration->arity()
				<< "” does not match any declared predicate";
	}

	// Detect integer variables
	if (context.performIntegerDetection)
		examineSemantics::detectIntegerVariables(completedFormulas);

	// Simplify output if specified
	if (performSimplification)
		for (auto &completedFormula : completedFormulas)
			examineSemantics::simplify(completedFormula);

	// Print specifiers for integer predicate parameters
	for (auto &predicateDeclaration : context.predicateDeclarations)
	{
		// Check that the predicate is used and not declared #external
		if (!predicateDeclaration->isUsed || predicateDeclaration->isExternal)
			continue;

		const auto isPredicateVisible =
			(predicateDeclaration->visibility == ast::PredicateDeclaration::Visibility::Visible)
			|| (predicateDeclaration->visibility == ast::PredicateDeclaration::Visibility::Default
				&& context.defaultPredicateVisibility == ast::PredicateDeclaration::Visibility::Visible);

		// If the predicate ought to be visible, don’t eliminate it
		if (!isPredicateVisible)
			continue;

		translationCommon::printTypeAnnotation(*predicateDeclaration, context, printContext);
	}

	// TODO: remove variables that are not referenced after simplification

	for (const auto &completedFormula : completedFormulas)
	{
		translationCommon::printFormula(completedFormula, translationCommon::FormulaType::Axiom, context, printContext);
		context.logger.outputStream() << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames, Context &context)
{
	if (fileNames.size() > 1)
		throw TranslationException("only one file may me translated at a time when examining semantics");

	std::vector<ast::ScopedFormula> scopedFormulas;

	const auto read =
		[&](const auto &rule)
		{
			examineSemantics::read(rule, context, scopedFormulas);
		};

	const auto readStatement =
		[&](auto &&statement) -> void
		{
			statement.data.accept(StatementVisitor<decltype(read), std::vector<ast::ScopedFormula>>(),
				statement, read, context, scopedFormulas);
		};

	translationCommon::readSingleFile(readStatement, fileNames.front(), context);

	translate(context, scopedFormulas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream, Context &context)
{
	std::vector<ast::ScopedFormula> scopedFormulas;

	const auto read =
		[&](const Clingo::AST::Rule &rule)
		{
			// TODO: remove unnecessary namespace specifier
			examineSemantics::read(rule, context, scopedFormulas);
		};

	const auto readStatement =
		[&](const Clingo::AST::Statement &statement)
		{
			statement.data.accept(StatementVisitor<decltype(read), std::vector<ast::ScopedFormula>>(), statement,
				read, context, scopedFormulas);
		};

	translationCommon::readSingleStream(readStatement, fileName, stream, context);

	translate(context, scopedFormulas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
