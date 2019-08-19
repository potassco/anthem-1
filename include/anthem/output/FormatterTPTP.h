#ifndef __ANTHEM__OUTPUT__FORMATTER_TPTP_H
#define __ANTHEM__OUTPUT__FORMATTER_TPTP_H

#include <cassert>

#include <anthem/AST.h>
#include <anthem/Exception.h>
#include <anthem/Utils.h>
#include <anthem/output/ColorStream.h>
#include <anthem/output/Formatter.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FormatterTPTP
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormatterTPTP
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Primitives
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::BinaryOperation::Operator operator_, PrintContext &, bool)
	{
		switch (operator_)
		{
			case ast::BinaryOperation::Operator::Plus:
				return (stream << output::Keyword(AuxiliaryFunctionNameSum));
			case ast::BinaryOperation::Operator::Minus:
				return (stream << output::Keyword(AuxiliaryFunctionNameDifference));
			case ast::BinaryOperation::Operator::Multiplication:
				return (stream << output::Keyword(AuxiliaryFunctionNameProduct));
			case ast::BinaryOperation::Operator::Division:
				throw TranslationException("division operator not implemented with TPTP");
			case ast::BinaryOperation::Operator::Modulo:
				throw TranslationException("modulo operator not implemented with TPTP");
			case ast::BinaryOperation::Operator::Power:
				throw TranslationException("power operator not implemented with TPTP");
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::BinaryOperation &binaryOperation, PrintContext &printContext, bool)
	{
		print(stream, binaryOperation.operator_, printContext, true);
		stream << "(";
		print(stream, binaryOperation.left, printContext, false);
		stream << ", ";
		print(stream, binaryOperation.right, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Boolean &boolean, PrintContext &, bool)
	{
		if (boolean.value)
			return (stream << output::Boolean("$true"));

		return (stream << output::Boolean("$false"));
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Comparison &comparison, PrintContext &printContext, bool)
	{
		if (comparison.operator_ == ast::Comparison::Operator::Equal || comparison.operator_ == ast::Comparison::Operator::NotEqual)
		{
			stream << "(";
			print(stream, comparison.left, printContext, false);

			if (comparison.operator_ == ast::Comparison::Operator::Equal)
				stream << " = ";
			else if (comparison.operator_ == ast::Comparison::Operator::NotEqual)
				stream << " != ";

			print(stream, comparison.right, printContext, false);
			stream << ")";

			return stream;
		}

		switch (comparison.operator_)
		{
			// TODO: rename and reorder for consistency
			case ast::Comparison::Operator::GreaterThan:
				stream << output::Keyword(AuxiliaryPredicateNameGreater);
				break;
			case ast::Comparison::Operator::LessThan:
				stream << output::Keyword(AuxiliaryPredicateNameLess);
				break;
			case ast::Comparison::Operator::LessEqual:
				stream << output::Keyword(AuxiliaryPredicateNameLessEqual);
				break;
			case ast::Comparison::Operator::GreaterEqual:
				stream << output::Keyword(AuxiliaryPredicateNameGreaterEqual);
				break;
			default:
				throw TranslationException("equality operators require infix notation, please report this to the bug tracker");
		}

		stream << "(";
		print(stream, comparison.left, printContext, false);
		stream << ", ";
		print(stream, comparison.right, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Function &function, PrintContext &printContext, bool)
	{
		stream << function.declaration->name;

		if (function.arguments.empty())
			return stream;

		stream << "(";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				stream << ", ";

			print(stream, *i, printContext, true);
		}

		if (function.declaration->name.empty() && function.arguments.size() == 1)
			stream << ",";

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::FunctionDeclaration &functionDeclaration, PrintContext &, bool)
	{
		return (stream << functionDeclaration.name << "/" << functionDeclaration.arity());
	}

	static output::ColorStream &print(output::ColorStream &, const ast::In &, PrintContext &, bool)
	{
		throw TranslationException("set inclusion operator not implemented with TPTP, please report to bug tracker");
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Integer &integer, PrintContext &, bool)
	{
		if (integer.value < 0)
			return (stream << output::Keyword("$uminus") << "(" << output::Number<int>(-integer.value) << ")");

		return (stream << output::Number<int>(integer.value));
	}

	static output::ColorStream &print(output::ColorStream &, const ast::Interval &, PrintContext &, bool)
	{
		throw TranslationException("intervals not implemented with TPTP, please report to bug tracker");
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Predicate &predicate, PrintContext &printContext, bool)
	{
		stream << predicate.declaration->name;

		if (predicate.arguments.empty())
			return stream;

		stream << "(";

		for (auto i = predicate.arguments.cbegin(); i != predicate.arguments.cend(); i++)
		{
			if (i != predicate.arguments.cbegin())
				stream << ", ";

			print(stream, *i, printContext, false);
		}

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::PredicateDeclaration &predicateDeclaration, PrintContext &, bool)
	{
		return (stream << predicateDeclaration.name << "/" << predicateDeclaration.arity());
	}

	static output::ColorStream &print(output::ColorStream &, const ast::SpecialInteger &, PrintContext &, bool)
	{
		throw TranslationException("special integers not implemented with TPTP");
	}

	static output::ColorStream &print(output::ColorStream &, const ast::String &, PrintContext &, bool)
	{
		throw TranslationException("strings not implemented with TPTP");
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::UnaryOperation &unaryOperation, PrintContext &printContext, bool)
	{
		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				throw TranslationException("absolute value not implemented with TPTP");
			case ast::UnaryOperation::Operator::Minus:
				stream << output::Keyword(AuxiliaryFunctionNameUnaryMinus) << "(";
				break;
		}

		print(stream, unaryOperation.argument, printContext, true);

		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				throw TranslationException("absolute value not implemented with TPTP");
			case ast::UnaryOperation::Operator::Minus:
				stream << ")";
				break;
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Variable &variable, PrintContext &printContext, bool)
	{
		assert(variable.declaration != nullptr);

		return print(stream, *variable.declaration, printContext, true);
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::VariableDeclaration &variableDeclaration, PrintContext &printContext, bool)
	{
		const auto printVariableDeclaration =
			[&stream, &variableDeclaration](const auto *prefix, auto &variableIDs) -> output::ColorStream &
			{
				auto matchingVariableID = variableIDs.find(&variableDeclaration);

				if (matchingVariableID == variableIDs.cend())
				{
					auto emplaceResult = variableIDs.emplace(std::make_pair(&variableDeclaration, variableIDs.size() + 1));
					assert(emplaceResult.second);
					matchingVariableID = emplaceResult.first;
				}

				const auto variableName = std::string(prefix) + std::to_string(matchingVariableID->second);

				return (stream << output::Variable(variableName.c_str()));
			};

		if (variableDeclaration.domain != Domain::Union)
			throw TranslationException("expected all variables to have union type, please report to bug tracker");

		switch (variableDeclaration.type)
		{
			case ast::VariableDeclaration::Type::UserDefined:
				printVariableDeclaration(UserVariablePrefix, printContext.userVariableIDs);
				break;
			default:
				printVariableDeclaration(BodyVariablePrefix, printContext.bodyVariableIDs);
				break;
		}

		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Expressions
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::And &and_, PrintContext &printContext, bool)
	{
		stream << "(";

		for (auto i = and_.arguments.cbegin(); i != and_.arguments.cend(); i++)
		{
			if (i != and_.arguments.cbegin())
				stream << " " << output::Operator("&") << " ";

			print(stream, *i, printContext, false);
		}

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Biconditional &biconditional, PrintContext &printContext, bool)
	{
		stream << "(";

		print(stream, biconditional.left, printContext, false);
		stream << " <=> ";
		print(stream, biconditional.right, printContext, false);

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Exists &exists, PrintContext &printContext, bool)
	{
		stream << "(" << output::Operator("?") << "[";

		for (auto i = exists.variables.cbegin(); i != exists.variables.cend(); i++)
		{
			const auto &variableDeclaration = **i;

			if (i != exists.variables.cbegin())
				stream << ", ";

			print(stream, variableDeclaration, printContext, true);
			stream << ": " << output::Keyword("object");
		}

		stream << "]: ";
		print(stream, exists.argument, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::ForAll &forAll, PrintContext &printContext, bool)
	{
		stream << "(" << output::Operator("!") << "[";

		for (auto i = forAll.variables.cbegin(); i != forAll.variables.cend(); i++)
		{
			const auto &variableDeclaration = **i;

			if (i != forAll.variables.cbegin())
				stream << ", ";

			print(stream, variableDeclaration, printContext, true);
			stream << ": " << output::Keyword("object");
		}

		stream << "]: ";
		print(stream, forAll.argument, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Implies &implies, PrintContext &printContext, bool)
	{
		stream << "(";
		print(stream, implies.antecedent, printContext, false);
		stream << " => ";
		print(stream, implies.consequent, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Not &not_, PrintContext &printContext, bool)
	{
		stream << "(" << output::Operator("~");
		print(stream, not_.argument, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Or &or_, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		for (auto i = or_.arguments.cbegin(); i != or_.arguments.cend(); i++)
		{
			if (i != or_.arguments.cbegin())
				stream << " " << output::Operator("|") << " ";

			print(stream, *i, printContext, true);
		}

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Variants
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::Formula &formula, PrintContext &printContext, bool omitParentheses)
	{
		return formula.accept(VariantPrintVisitor<FormatterTPTP, ast::Formula>(), stream, printContext, omitParentheses);
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Term &term, PrintContext &printContext, bool omitParentheses)
	{
		return term.accept(VariantPrintVisitor<FormatterTPTP, ast::Term>(), stream, printContext, omitParentheses);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
