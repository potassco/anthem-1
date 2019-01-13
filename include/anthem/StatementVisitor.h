#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/AST.h>
#include <anthem/ASTCopy.h>
#include <anthem/Body.h>
#include <anthem/BodyDirect.h>
#include <anthem/Head.h>
#include <anthem/HeadDirect.h>
#include <anthem/RuleContext.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces empty and 1-element conjunctions in the antecedent of normal-form formulas
inline void reduce(ast::Implies &implies)
{
	if (!implies.antecedent.is<ast::And>())
		return;

	auto &antecedent = implies.antecedent.get<ast::And>();

	// Use “true” as the consequent in case it is empty
	if (antecedent.arguments.empty())
		implies.antecedent = ast::Boolean(true);
	else if (antecedent.arguments.size() == 1)
		implies.antecedent = std::move(antecedent.arguments[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Translating the head directly doesn’t allow for later completion but leads to a simpler result
void translateRuleDirectly(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &, std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
{
	RuleContext ruleContext;
	ast::VariableStack variableStack;
	variableStack.push(&ruleContext.freeVariables);

	// Directly translate the head
	auto consequent = rule.head.data.accept(direct::HeadLiteralTranslateToConsequentVisitor(), rule.head, context, ruleContext, variableStack);

	ast::And antecedent;

	// Translate body literals
	for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
	{
		const auto &bodyLiteral = *i;

		auto argument = bodyLiteral.data.accept(direct::BodyBodyLiteralTranslateVisitor(), bodyLiteral, context, ruleContext, variableStack);
		antecedent.arguments.emplace_back(std::move(argument));
	}

	ast::Implies formula(std::move(antecedent), std::move(consequent));
	ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
	scopedFormulas.emplace_back(std::move(scopedFormula));
	reduce(scopedFormulas.back().formula.get<ast::Implies>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translateRuleForCompletion(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &, std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
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
	consequent = rule.head.data.accept(HeadLiteralTranslateToConsequentVisitor(), rule.head, ruleContext, context, headVariableIndex);

	assert(ruleContext.headTerms.size() == headVariableIndex - ruleContext.headVariablesStartIndex);

	if (!consequent)
		throw TranslationException(rule.head.location, "could not translate formula consequent");

	// Generate auxiliary variables replacing the head atom’s arguments
	for (auto i = ruleContext.headTerms.cbegin(); i != ruleContext.headTerms.cend(); i++)
	{
		const auto &headTerm = **i;

		const auto auxiliaryHeadVariableID = ruleContext.headVariablesStartIndex + i - ruleContext.headTerms.cbegin();
		auto element = ast::Variable(ruleContext.freeVariables[auxiliaryHeadVariableID].get());
		auto set = translate(headTerm, ruleContext, context, variableStack);
		auto in = ast::In(std::move(element), std::move(set));

		antecedent.arguments.emplace_back(std::move(in));
	}

	// Translate body literals
	for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
	{
		const auto &bodyLiteral = *i;

		auto argument = bodyLiteral.data.accept(BodyBodyLiteralTranslateVisitor(), bodyLiteral, ruleContext, context, variableStack);

		if (!argument)
			throw TranslationException(bodyLiteral.location, "could not translate body literal");

		antecedent.arguments.emplace_back(std::move(argument.value()));
	}

	if (!ruleContext.isChoiceRule)
	{
		ast::Implies formula(std::move(antecedent), std::move(consequent.value()));
		ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
		scopedFormulas.emplace_back(std::move(scopedFormula));
		reduce(scopedFormulas.back().formula.get<ast::Implies>());
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

				reduce(implies);
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

////////////////////////////////////////////////////////////////////////////////////////////////////

struct StatementVisitor
{
	void visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		context.logger.log(output::Priority::Debug, statement.location) << "reading program “" << program.name << "”";

		if (std::strcmp(program.name, "base") != 0)
			throw LogicException(statement.location, "program parts currently unsupported");

		if (!program.parameters.empty())
			throw LogicException(statement.location, "program parameters currently unsupported");
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
	{
		context.logger.log(output::Priority::Debug, statement.location) << "reading rule";

		switch (context.headTranslationMode)
		{
			case HeadTranslationMode::Direct:
				translateRuleDirectly(rule, statement, scopedFormulas, context);
				break;
			case HeadTranslationMode::ForCompletion:
				translateRuleForCompletion(rule, statement, scopedFormulas, context);
				break;
		}
	}

	void visit(const Clingo::AST::ShowSignature &showSignature, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		if (showSignature.csp)
			throw LogicException(statement.location, "CSP #show statements are not supported");

		auto &signature = showSignature.signature;

		if (signature.negative())
			throw LogicException(statement.location, "negative #show atom signatures are currently unsupported");

		context.showStatementsUsed = true;
		context.defaultPredicateVisibility = ast::PredicateDeclaration::Visibility::Hidden;

		if (std::strlen(signature.name()) == 0)
		{
			context.logger.log(output::Priority::Debug, statement.location) << "showing no predicates by default";
			return;
		}

		context.logger.log(output::Priority::Debug, statement.location) << "showing “" << signature.name() << "/" << signature.arity() << "”";

		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(signature.name(), signature.arity());
		predicateDeclaration->visibility = ast::PredicateDeclaration::Visibility::Visible;
	}

	void visit(const Clingo::AST::ShowTerm &, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &)
	{
		throw LogicException(statement.location, "only #show statements for atoms (not terms) are supported currently");
	}

	void visit(const Clingo::AST::External &external, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		const auto fail =
			[&]()
			{
				throw LogicException(statement.location, "only #external declarations of the form “#external <predicate name>(<arity>).” or “#external integer(<function name>(<arity>)).” supported");
			};

		if (!external.body.empty())
			fail();

		if (!external.atom.data.is<Clingo::AST::Function>())
			fail();

		const auto &predicate = external.atom.data.get<Clingo::AST::Function>();

		if (predicate.arguments.size() != 1)
			fail();

		const auto handleIntegerDeclaration =
			[&]()
			{
				// Integer function declarations are treated separately if applicable
				if (strcmp(predicate.name, "integer") != 0)
					return false;

				if (predicate.arguments.size() != 1)
					return false;

				const auto &functionArgument = predicate.arguments.front();

				if (!functionArgument.data.is<Clingo::AST::Function>())
					return false;

				const auto &function = functionArgument.data.get<Clingo::AST::Function>();

				if (function.arguments.size() != 1)
					return false;

				const auto &arityArgument = function.arguments.front();

				if (!arityArgument.data.is<Clingo::Symbol>())
					return false;

				const auto &aritySymbol = arityArgument.data.get<Clingo::Symbol>();

				if (aritySymbol.type() != Clingo::SymbolType::Number)
					return false;

				const size_t arity = aritySymbol.number();

				auto functionDeclaration = context.findOrCreateFunctionDeclaration(function.name, arity);
				functionDeclaration->domain = Domain::Integer;

				return true;
			};

		if (handleIntegerDeclaration())
			return;

		const auto &arityArgument = predicate.arguments.front();

		if (!arityArgument.data.is<Clingo::Symbol>())
			fail();

		const auto &aritySymbol = arityArgument.data.get<Clingo::Symbol>();

		if (aritySymbol.type() != Clingo::SymbolType::Number)
			fail();

		context.externalStatementsUsed = true;

		const size_t arity = aritySymbol.number();

		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(predicate.name, arity);
		predicateDeclaration->isExternal = true;
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &)
	{
		throw LogicException(statement.location, "statement currently unsupported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
