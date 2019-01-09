#ifndef __ANTHEM__OUTPUT__FORMATTER_HUMAN_READABLE_H
#define __ANTHEM__OUTPUT__FORMATTER_HUMAN_READABLE_H

#include <cassert>

#include <anthem/AST.h>
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
// FormatterHumanReadable
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormatterHumanReadable
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Primitives
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, ast::BinaryOperation::Operator operator_, PrintContext &, bool)
	{
		switch (operator_)
		{
			case ast::BinaryOperation::Operator::Plus:
				return (stream << output::Operator("+"));
			case ast::BinaryOperation::Operator::Minus:
				return (stream << output::Operator("-"));
			case ast::BinaryOperation::Operator::Multiplication:
				return (stream << output::Operator("*"));
			case ast::BinaryOperation::Operator::Division:
				return (stream << output::Operator("/"));
			case ast::BinaryOperation::Operator::Modulo:
				return (stream << output::Operator("%"));
			case ast::BinaryOperation::Operator::Power:
				return (stream << output::Operator("**"));
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::BinaryOperation &binaryOperation, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		print(stream, binaryOperation.left, printContext, false);
		stream << " ";
		print(stream, binaryOperation.operator_, printContext, true);
		stream << " ";
		print(stream, binaryOperation.right, printContext, false);

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Boolean &boolean, PrintContext &, bool)
	{
		if (boolean.value)
			return (stream << output::Boolean("#true"));

		return (stream << output::Boolean("#false"));
	}

	static output::ColorStream &print(output::ColorStream &stream, ast::Comparison::Operator operator_, PrintContext &, bool)
	{
		switch (operator_)
		{
			case ast::Comparison::Operator::GreaterThan:
				return (stream << output::Operator(">"));
			case ast::Comparison::Operator::LessThan:
				return (stream << output::Operator("<"));
			case ast::Comparison::Operator::LessEqual:
				return (stream << output::Operator("<="));
			case ast::Comparison::Operator::GreaterEqual:
				return (stream << output::Operator(">="));
			case ast::Comparison::Operator::NotEqual:
				return (stream << output::Operator("!="));
			case ast::Comparison::Operator::Equal:
				return (stream << output::Operator("="));
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Comparison &comparison, PrintContext &printContext, bool)
	{
		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		print(stream, comparison.left, printContext, false);
		stream << " ";
		print(stream, comparison.operator_, printContext, true);
		stream << " ";
		print(stream, comparison.right, printContext, false);

		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
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

	static output::ColorStream &print(output::ColorStream &stream, const ast::In &in, PrintContext &printContext, bool)
	{
		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		print(stream, in.element, printContext, false);
		stream << " " << output::Keyword("in") << " ";
		print(stream, in.set, printContext, false);

		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Integer &integer, PrintContext &, bool)
	{
		return (stream << output::Number<int>(integer.value));
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Interval &interval, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		print(stream, interval.from, printContext, false);
		stream << "..";
		print(stream, interval.to, printContext, false);

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
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

			print(stream, *i, printContext, true);
		}

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::PredicateDeclaration &predicateDeclaration, PrintContext &, bool)
	{
		return (stream << predicateDeclaration.name << "/" << predicateDeclaration.arity());
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::SpecialInteger &specialInteger, PrintContext &, bool)
	{
		switch (specialInteger.type)
		{
			case ast::SpecialInteger::Type::Infimum:
				return (stream << output::Number<std::string>("#inf"));
			case ast::SpecialInteger::Type::Supremum:
				return (stream << output::Number<std::string>("#sup"));
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::String &string, PrintContext &, bool)
	{
		return (stream << output::String(string.text.c_str()));
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::UnaryOperation &unaryOperation, PrintContext &printContext, bool)
	{
		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				stream << "|";
				break;
			case ast::UnaryOperation::Operator::Minus:
				stream << "-";
				break;
		}

		print(stream, unaryOperation.argument, printContext, true);

		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				stream << "|";
				break;
			case ast::UnaryOperation::Operator::Minus:
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

		if (variableDeclaration.domain == Domain::Integer)
			return printVariableDeclaration(IntegerVariablePrefix, printContext.integerVariableIDs);

		switch (variableDeclaration.type)
		{
			case ast::VariableDeclaration::Type::UserDefined:
				return printVariableDeclaration(UserVariablePrefix, printContext.userVariableIDs);
			case ast::VariableDeclaration::Type::Head:
				return printVariableDeclaration(HeadVariablePrefix, printContext.headVariableIDs);
			case ast::VariableDeclaration::Type::Body:
				return printVariableDeclaration(BodyVariablePrefix, printContext.bodyVariableIDs);
		}

		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Expressions
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::And &and_, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		for (auto i = and_.arguments.cbegin(); i != and_.arguments.cend(); i++)
		{
			if (i != and_.arguments.cbegin())
				stream << " " << output::Keyword("and") << " ";

			print(stream, *i, printContext, false);
		}

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Biconditional &biconditional, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		print(stream, biconditional.left, printContext, false);
		stream << " <-> ";
		print(stream, biconditional.right, printContext, false);

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Exists &exists, PrintContext &printContext, bool)
	{
		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		stream << output::Keyword("exists") << " ";

		for (auto i = exists.variables.cbegin(); i != exists.variables.cend(); i++)
		{
			if (i != exists.variables.cbegin())
				stream << ", ";

			print(stream, **i, printContext, true);
		}

		stream << " ";
		print(stream, exists.argument, printContext, false);

		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::ForAll &forAll, PrintContext &printContext, bool)
	{
		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		stream << output::Keyword("forall") << " ";

		for (auto i = forAll.variables.cbegin(); i != forAll.variables.cend(); i++)
		{
			if (i != forAll.variables.cbegin())
				stream << ", ";

			print(stream, **i, printContext, true);
		}

		stream << " ";
		print(stream, forAll.argument, printContext, false);

		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Implies &implies, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		print(stream, implies.antecedent, printContext, false);
		stream << " -> ";
		print(stream, implies.consequent, printContext, false);

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Not &not_, PrintContext &printContext, bool)
	{
		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		stream << output::Keyword("not") << " ";
		print(stream, not_.argument, printContext, false);

		if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
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
				stream << " " << output::Keyword("or") << " ";

			print(stream, *i, printContext, false);
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
		return formula.accept(VariantPrintVisitor<FormatterHumanReadable, ast::Formula>(), stream, printContext, omitParentheses);
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Term &term, PrintContext &printContext, bool omitParentheses)
	{
		return term.accept(VariantPrintVisitor<FormatterHumanReadable, ast::Term>(), stream, printContext, omitParentheses);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
