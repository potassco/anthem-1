#ifndef __ANTHEM__BODY_H
#define __ANTHEM__BODY_H

#include <algorithm>

#include <anthem/Context.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Body
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermPrintVisitor
{
	void visit(const Clingo::Symbol &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“symbol” terms currently unsupported in this context", context);
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported in this context", context);
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported in this context", context);
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported in this context", context);
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported in this context", context);
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Literal &literal, const Clingo::AST::Term &term, Context &context)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated literals currently unsupported", context);

		if (function.arguments.empty())
		{
			std::cout << function.name;
			return;
		}

		std::cout << "exists ";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				std::cout << ", ";

			std::cout << AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID + (i - function.arguments.cbegin()));
		}

		std::cout << " (";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			const auto &argument = *i;

			if (i != function.arguments.cbegin())
				std::cout << " and ";

			std::cout
				<< AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID + (i - function.arguments.cbegin()))
				<< " in "
				<< argument;
		}

		std::cout << " and ";

		if (literal.sign == Clingo::AST::Sign::Negation)
			std::cout << "not ";

		std::cout << function.name << "(";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				std::cout << ", ";

			std::cout << AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID + (i - function.arguments.cbegin()));
		}

		std::cout << "))";

		context.auxiliaryBodyLiteralID += function.arguments.size();
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralPrintVisitor
{
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "“boolean” literals currently unsupported in this context", context);
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, Context &context)
	{
		term.data.accept(TermPrintVisitor(), literal, term, context);
	}

	void visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, Context &context)
	{
		assert(literal.sign == Clingo::AST::Sign::None);

		const char *operatorName = "";

		switch (comparison.comparison)
		{
			case Clingo::AST::ComparisonOperator::GreaterThan:
				operatorName = ">";
				break;
			case Clingo::AST::ComparisonOperator::LessThan:
				operatorName = "<";
				break;
			case Clingo::AST::ComparisonOperator::LessEqual:
				operatorName = "<=";
				break;
			case Clingo::AST::ComparisonOperator::GreaterEqual:
				operatorName = ">=";
				break;
			case Clingo::AST::ComparisonOperator::NotEqual:
				operatorName = "!=";
				break;
			case Clingo::AST::ComparisonOperator::Equal:
				operatorName = "=";
				break;
		}

		std::cout
			<< "exists " << AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID)
			<< ", " << AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID + 1)
			<< " ("
			<< AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID)
			<< " in " << comparison.left
			<< " and "
			<< AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID + 1)
			<< " in " << comparison.right
			<< " and "
			<< AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID)
			<< operatorName
			<< AuxiliaryBodyVariablePrefix << (context.auxiliaryBodyLiteralID + 1)
			<< ")";

		context.auxiliaryBodyLiteralID += 2;
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "CSP literals currently unsupported", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralPrintVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &, Context &context)
	{
		literal.data.accept(LiteralPrintVisitor(), literal, context);
	}

	void visit(const Clingo::AST::ConditionalLiteral &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“conditional literal” body literals currently unsupported", context);
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“aggregate” body literals currently unsupported", context);
	}

	void visit(const Clingo::AST::BodyAggregate &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“body aggregate” body literals currently unsupported", context);
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“theory atom” body literals currently unsupported", context);
	}

	void visit(const Clingo::AST::Disjoint &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context)
	{
		throwErrorAtLocation(bodyLiteral.location, "“disjoint” body literals currently unsupported", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
