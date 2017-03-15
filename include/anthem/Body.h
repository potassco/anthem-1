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

ast::VariablePointer makeAuxiliaryBodyVariable(const int i)
{
	auto variableName = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(i);

	return std::make_unique<ast::Variable>(std::move(variableName), ast::Variable::Type::Reserved);
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyTermTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::Symbol &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“symbol” terms currently unsupported in this context", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Variable &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported in this context", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported in this context", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported in this context", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Interval &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported in this context", context);
		return std::experimental::nullopt;
	}

	// TODO: refactor
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Literal &literal, const Clingo::AST::Term &, Context &context)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated literals currently unsupported", context);

		if (function.arguments.empty())
			return std::make_unique<ast::Predicate>(std::string(function.name));

		std::vector<ast::VariablePointer> variables;
		variables.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			variables.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID + i));

		auto conjunction = std::make_unique<ast::And>();

		for (size_t i = 0; i < function.arguments.size(); i++)
		{
			const auto &argument = function.arguments[i];
			conjunction->arguments.emplace_back(std::make_unique<ast::In>(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID + i), translate(argument, context)));
		}

		auto predicate = std::make_unique<ast::Predicate>(std::string(function.name));
		predicate->arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			predicate->arguments.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID + i));

		if (literal.sign == Clingo::AST::Sign::None)
			conjunction->arguments.emplace_back(std::move(predicate));
		else
			conjunction->arguments.emplace_back(std::make_unique<ast::Not>(std::move(predicate)));

		context.auxiliaryBodyLiteralID += function.arguments.size();

		return std::make_unique<ast::Exists>(std::move(variables), std::move(conjunction));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Pool &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "“boolean” literals currently unsupported in this context", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, Context &context)
	{
		return term.data.accept(BodyTermTranslateVisitor(), literal, term, context);
		return std::experimental::nullopt;
	}

	// TODO: refactor
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, Context &context)
	{
		assert(literal.sign == Clingo::AST::Sign::None);

		const auto operator_ = translate(comparison.comparison);

		std::vector<ast::VariablePointer> variables;
		variables.reserve(2);
		variables.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID));
		variables.emplace_back(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID + 1));

		auto conjunction = std::make_unique<ast::And>();
		conjunction->arguments.reserve(3);
		conjunction->arguments.emplace_back(std::make_unique<ast::In>(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID), translate(comparison.left, context)));
		conjunction->arguments.emplace_back(std::make_unique<ast::In>(makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID + 1), translate(comparison.right, context)));
		conjunction->arguments.emplace_back(std::make_unique<ast::Comparison>(operator_, makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID), makeAuxiliaryBodyVariable(context.auxiliaryBodyLiteralID + 1)));

		context.auxiliaryBodyLiteralID += 2;

		return std::make_unique<ast::Exists>(std::move(variables), std::move(conjunction));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "CSP literals currently unsupported", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyBodyLiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &, Context &context)
	{
		return literal.data.accept(BodyLiteralTranslateVisitor(), literal, context);
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::ConditionalLiteral &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“conditional literal” body literals currently unsupported", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Aggregate &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“aggregate” body literals currently unsupported", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::BodyAggregate &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“body aggregate” body literals currently unsupported", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“theory atom” body literals currently unsupported", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Disjoint &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“disjoint” body literals currently unsupported", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
