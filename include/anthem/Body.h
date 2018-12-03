#ifndef __ANTHEM__BODY_H
#define __ANTHEM__BODY_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ComparisonOperator.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Body
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyTermTranslateVisitor
{
	// TODO: refactor
	std::optional<ast::Formula> visit(const Clingo::AST::Function &function,
		const Clingo::AST::Literal &literal, const Clingo::AST::Term &, RuleContext &ruleContext,
		Context &context, ast::VariableStack &variableStack)
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
				return ast::Not(ast::Predicate(predicateDeclaration));
		}

		// Create new body variable declarations
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

		// TODO: implement pushing with scoped guards to avoid bugs
		variableStack.push(&parameters);

		ast::And conjunction;

		for (size_t i = 0; i < function.arguments.size(); i++)
		{
			auto &argument = function.arguments[i];
			conjunction.arguments.emplace_back(ast::In(ast::Variable(parameters[i].get()), translate(argument, ruleContext, context, variableStack)));
		}

		variableStack.pop();

		ast::Predicate predicate(predicateDeclaration);
		predicate.arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			predicate.arguments.emplace_back(ast::Variable(parameters[i].get()));

		if (literal.sign == Clingo::AST::Sign::None)
			conjunction.arguments.emplace_back(std::move(predicate));
		else if (literal.sign == Clingo::AST::Sign::Negation)
			conjunction.arguments.emplace_back(ast::Not(std::move(predicate)));

		return ast::Exists(std::move(parameters), std::move(conjunction));
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &,
		const Clingo::AST::Term &term, RuleContext &, Context &, ast::VariableStack &)
	{
		assert(!term.data.is<Clingo::AST::Function>());

		throw TranslationException(term.location, "term currently unsupported in this context, expected function");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralTranslateVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, RuleContext &, Context &, ast::VariableStack &)
	{
		return ast::Boolean(boolean.value);
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		return term.data.accept(BodyTermTranslateVisitor(), literal, term, ruleContext, context, variableStack);
	}

	// TODO: refactor
	std::optional<ast::Formula> visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		// Comparisons should never have a sign, because these are converted to positive comparisons by clingo
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated comparisons currently unsupported");

		const auto operator_ = translate(comparison.comparison);

		ast::VariableDeclarationPointers parameters;
		parameters.reserve(2);
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

		ast::And conjunction;
		conjunction.arguments.reserve(3);
		conjunction.arguments.emplace_back(ast::In(ast::Variable(parameters[0].get()), translate(comparison.left, ruleContext, context, variableStack)));
		conjunction.arguments.emplace_back(ast::In(ast::Variable(parameters[1].get()), translate(comparison.right, ruleContext, context, variableStack)));
		conjunction.arguments.emplace_back(ast::Comparison(operator_, ast::Variable(parameters[0].get()), ast::Variable(parameters[1].get())));

		return ast::Exists(std::move(parameters), std::move(conjunction));
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &literal, RuleContext &, Context &, ast::VariableStack &)
	{
		throw TranslationException(literal.location, "literal currently unsupported in this context, expected function or term");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyBodyLiteralTranslateVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &bodyLiteral, RuleContext &ruleContext, Context &context, ast::VariableStack &variableStack)
	{
		if (bodyLiteral.sign != Clingo::AST::Sign::None)
			throw TranslationException(bodyLiteral.location, "only positive body literals supported currently");

		return literal.data.accept(BodyLiteralTranslateVisitor(), literal, ruleContext, context, variableStack);
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::BodyLiteral &bodyLiteral, RuleContext &, Context &, ast::VariableStack &)
	{
		throw TranslationException(bodyLiteral.location, "body literal currently unsupported in this context, expected literal");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
