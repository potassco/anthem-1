#ifndef __ANTHEM__HEAD_DIRECT_H
#define __ANTHEM__HEAD_DIRECT_H

#include <algorithm>
#include <optional>

#include <anthem/AST.h>
#include <anthem/ComparisonOperator.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HeadDirect
//
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Translate Head directly
////////////////////////////////////////////////////////////////////////////////////////////////////

struct FunctionTermTranslateDirectlyVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throw TranslationException(literal.location, "double-negated literals currently unsupported");

		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
		predicateDeclaration->isUsed = true;

		if (function.arguments.empty())
		{
			if (literal.sign == Clingo::AST::Sign::None)
				return ast::Predicate(predicateDeclaration);
			else if (literal.sign == Clingo::AST::Sign::Negation)
				return ast::Formula::make<ast::Not>(ast::Predicate(predicateDeclaration));
		}

		// Create new body variable declarations
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head));

		// TODO: implement pushing with scoped guards to avoid bugs
		variableStack.push(&parameters);

		ast::And conjunction;

		for (size_t i = 0; i < function.arguments.size(); i++)
		{
			auto &argument = function.arguments[i];
			conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(ast::Variable(parameters[i].get()), translate(argument, ruleContext, context, variableStack)));
		}

		variableStack.pop();

		ast::Predicate predicate(predicateDeclaration);
		predicate.arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			predicate.arguments.emplace_back(ast::Variable(parameters[i].get()));

		auto antecedent = std::move(conjunction);

		auto consequent = (literal.sign == Clingo::AST::Sign::None)
			? std::move(predicate)
			: ast::Formula::make<ast::Not>(std::move(predicate));

		ast::Implies implies(std::move(antecedent), std::move(consequent));

		return ast::Formula::make<ast::ForAll>(std::move(parameters), std::move(implies));
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::Term &term, const Clingo::AST::Literal &, RuleContext &, Context &, ast::VariableStack &)
	{
		throw TranslationException(term.location, "term currently unsupported in this context, function expected");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralTranslateDirectlyVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &literal, RuleContext &, Context &, ast::VariableStack &)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throw TranslationException(literal.location, "double-negated head literals currently unsupported");

		const auto value = (literal.sign == Clingo::AST::Sign::None) ? boolean.value : !boolean.value;

		return ast::Formula::make<ast::Boolean>(value);
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		// Comparisons should never have a sign, because these are converted to positive comparisons by clingo
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated comparisons currently unsupported");

		const auto operator_ = translate(comparison.comparison);

		ast::VariableDeclarationPointers parameters;
		parameters.reserve(2);
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head));
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head));

		ast::And conjunction;
		conjunction.arguments.reserve(3);
		conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(ast::Variable(parameters[0].get()), translate(comparison.left, ruleContext, context, variableStack)));
		conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(ast::Variable(parameters[1].get()), translate(comparison.right, ruleContext, context, variableStack)));
		conjunction.arguments.emplace_back(ast::Formula::make<ast::Comparison>(operator_, ast::Variable(parameters[0].get()), ast::Variable(parameters[1].get())));

		return ast::Formula::make<ast::ForAll>(std::move(parameters), std::move(conjunction));
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		return term.data.accept(FunctionTermTranslateDirectlyVisitor(), term, literal, ruleContext, context, variableStack);
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &literal, RuleContext &, Context &, ast::VariableStack &)
	{
		throw TranslationException(literal.location, "only disjunctions and comparisons of literals allowed as head literals");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralTranslateDirectlyToConsequentVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throw TranslationException(literal.location, "double-negated head literals currently unsupported");

		return literal.data.accept(LiteralTranslateDirectlyVisitor(), literal, ruleContext, context, variableStack);
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		std::vector<ast::Formula> arguments;
		arguments.reserve(disjunction.elements.size());

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throw TranslationException(headLiteral.location, "conditional head literals currently unsupported");

			auto argument = visit(conditionalLiteral.literal, headLiteral, ruleContext, context, variableStack);

			if (!argument)
				throw TranslationException(headLiteral.location, "could not parse argument");

			arguments.emplace_back(std::move(argument.value()));
		}

		return ast::Formula::make<ast::Or>(std::move(arguments));
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		throw TranslationException(headLiteral.location, "aggregates currently unsupported");
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &, Context &, ast::VariableStack &)
	{
		throw TranslationException(headLiteral.location, "head literal currently unsupported in this context, expected literal, disjunction, or aggregate");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
