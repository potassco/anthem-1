#ifndef __ANTHEM__BODY_H
#define __ANTHEM__BODY_H

#include <algorithm>

#include <anthem/AST.h>
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

ast::Comparison::Operator translate(Clingo::AST::ComparisonOperator comparisonOperator)
{
	switch (comparisonOperator)
	{
		case Clingo::AST::ComparisonOperator::GreaterThan:
			return ast::Comparison::Operator::GreaterThan;
		case Clingo::AST::ComparisonOperator::LessThan:
			return ast::Comparison::Operator::LessThan;
		case Clingo::AST::ComparisonOperator::LessEqual:
			return ast::Comparison::Operator::LessEqual;
		case Clingo::AST::ComparisonOperator::GreaterEqual:
			return ast::Comparison::Operator::GreaterEqual;
		case Clingo::AST::ComparisonOperator::NotEqual:
			return ast::Comparison::Operator::NotEqual;
		case Clingo::AST::ComparisonOperator::Equal:
			return ast::Comparison::Operator::Equal;
	}

	return ast::Comparison::Operator::NotEqual;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Variable makeAuxiliaryBodyVariable(int i)
{
	auto variableName = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(i);

	return ast::Variable(std::move(variableName), ast::Variable::Type::Reserved);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyTermTranslateVisitor
{
	// TODO: refactor
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Literal &literal, const Clingo::AST::Term &, Context &context)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated literals currently unsupported", context);

		if (function.arguments.empty())
		{
			auto predicate = ast::Formula::make<ast::Predicate>(std::string(function.name));

			if (literal.sign == Clingo::AST::Sign::None)
				return std::move(predicate);
			else if (literal.sign == Clingo::AST::Sign::Negation)
				return ast::Formula::make<ast::Not>(std::move(predicate));
		}

		std::vector<ast::Variable> variables;
		variables.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			variables.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID + i));

		ast::And conjunction;

		for (size_t i = 0; i < function.arguments.size(); i++)
		{
			const auto &argument = function.arguments[i];
			conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID + i), translate(argument, context)));
		}

		ast::Predicate predicate(std::string(function.name));
		predicate.arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			predicate.arguments.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID + i));

		if (literal.sign == Clingo::AST::Sign::None)
			conjunction.arguments.emplace_back(std::move(predicate));
		else if (literal.sign == Clingo::AST::Sign::Negation)
			conjunction.arguments.emplace_back(ast::Formula::make<ast::Not>(std::move(predicate)));

		context.auxiliaryBodyVariableID += function.arguments.size();

		return ast::Formula::make<ast::Exists>(std::move(variables), std::move(conjunction));
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "term currently unsupported in this context, expected function", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, Context &)
	{
		return ast::Formula::make<ast::Boolean>(boolean.value);
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, Context &context)
	{
		return term.data.accept(BodyTermTranslateVisitor(), literal, term, context);
	}

	// TODO: refactor
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, Context &context)
	{
		// Comparisons should never have a sign, because these are converted to positive comparisons by clingo
		if (literal.sign != Clingo::AST::Sign::None)
			throwErrorAtLocation(literal.location, "negated comparisons currently unsupported", context);

		const auto operator_ = translate(comparison.comparison);

		std::vector<ast::Variable> variables;
		variables.reserve(2);
		variables.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID));
		variables.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID + 1));

		ast::And conjunction;
		conjunction.arguments.reserve(3);
		conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID), translate(comparison.left, context)));
		conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID + 1), translate(comparison.right, context)));
		conjunction.arguments.emplace_back(ast::Formula::make<ast::Comparison>(operator_, makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID), makeAuxiliaryBodyVariable(context.auxiliaryBodyVariableID + 1)));

		context.auxiliaryBodyVariableID += 2;

		return ast::Formula::make<ast::Exists>(std::move(variables), std::move(conjunction));
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "literal currently unsupported in this context, expected function or term", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyBodyLiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		if (bodyLiteral.sign != Clingo::AST::Sign::None)
			throwErrorAtLocation(bodyLiteral.location, "only positive body literals supported currently", context);

		return literal.data.accept(BodyLiteralTranslateVisitor(), literal, context);
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "body literal currently unsupported in this context, expected literal", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
