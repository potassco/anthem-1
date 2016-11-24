#ifndef __ANTHEM__BODY_H
#define __ANTHEM__BODY_H

#include <algorithm>

#include <anthem/Context.h>
#include <anthem/Utils.h>
#include <anthem/output/ClingoOutput.h>
#include <anthem/output/Formatting.h>

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

	// TODO: check correctness
	void visit(const Clingo::AST::Function &function, const Clingo::AST::Literal &literal, const Clingo::AST::Term &term, Context &context)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated literals currently unsupported", context);

		auto &outputStream = context.logger.outputStream();

		if (function.arguments.empty())
		{
			outputStream << output::Function(function.name);
			return;
		}

		outputStream << output::Keyword("exists") << " ";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				outputStream << ", ";

			const auto variableName = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(i - function.arguments.cbegin());

			outputStream << output::Variable(variableName.c_str());
		}

		outputStream << " (";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			const auto &argument = *i;

			if (i != function.arguments.cbegin())
				outputStream << " " << Clingo::AST::BinaryOperator::And << " ";

			const auto variableName = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(context.auxiliaryBodyLiteralID + (i - function.arguments.cbegin()));

			outputStream
				<< output::Variable(variableName.c_str())
				<< " " << output::Keyword("in") << " "
				<< argument;
		}

		outputStream << " " << Clingo::AST::BinaryOperator::And << " ";

		if (literal.sign == Clingo::AST::Sign::Negation)
			std::cout << Clingo::AST::Sign::Negation << " ";

		outputStream << output::Function(function.name) << "(";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				outputStream << ", ";

			const auto variableName = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(context.auxiliaryBodyLiteralID + (i - function.arguments.cbegin()));

			outputStream << output::Variable(variableName.c_str());
		}

		outputStream << "))";

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

	// TODO: refactor
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

		const auto variableName1 = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(context.auxiliaryBodyLiteralID);
		const auto variableName2 = std::string(AuxiliaryBodyVariablePrefix) + std::to_string(context.auxiliaryBodyLiteralID + 1);

		context.logger.outputStream()
			<< output::Keyword("exists") << " " << output::Variable(variableName1.c_str())
			<< ", " << output::Variable(variableName2.c_str())
			<< " ("
			<< output::Variable(variableName1.c_str())
			<< " " << output::Keyword("in") << " " << comparison.left
			<< " " << Clingo::AST::BinaryOperator::And << " "
			<< output::Variable(variableName2.c_str())
			<< " " << output::Keyword("in") << " " << comparison.right
			<< " " << Clingo::AST::BinaryOperator::And << " "
			<< output::Variable(variableName1.c_str())
			<< " " << output::Operator(operatorName) << " "
			<< output::Variable(variableName2.c_str())
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
