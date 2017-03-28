#ifndef __ANTHEM__OUTPUT__AST_H
#define __ANTHEM__OUTPUT__AST_H

#include <cassert>

#include <anthem/AST.h>
#include <anthem/Utils.h>
#include <anthem/output/ColorStream.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST
//
////////////////////////////////////////////////////////////////////////////////////////////////////

output::ColorStream &operator<<(output::ColorStream &stream, BinaryOperation::Operator operator_);
output::ColorStream &operator<<(output::ColorStream &stream, const BinaryOperation &binaryOperation);
output::ColorStream &operator<<(output::ColorStream &stream, const Boolean &boolean);
output::ColorStream &operator<<(output::ColorStream &stream, const Comparison &comparison);
output::ColorStream &operator<<(output::ColorStream &stream, const Constant &constant);
output::ColorStream &operator<<(output::ColorStream &stream, const Function &function);
output::ColorStream &operator<<(output::ColorStream &stream, const In &in);
output::ColorStream &operator<<(output::ColorStream &stream, const Integer &integer);
output::ColorStream &operator<<(output::ColorStream &stream, const Interval &interval);
output::ColorStream &operator<<(output::ColorStream &stream, const Predicate &predicate);
output::ColorStream &operator<<(output::ColorStream &stream, const SpecialInteger &specialInteger);
output::ColorStream &operator<<(output::ColorStream &stream, const String &string);
output::ColorStream &operator<<(output::ColorStream &stream, const Variable &variable);

output::ColorStream &operator<<(output::ColorStream &stream, const And &and_);
output::ColorStream &operator<<(output::ColorStream &stream, const Biconditional &biconditional);
output::ColorStream &operator<<(output::ColorStream &stream, const Exists &exists);
output::ColorStream &operator<<(output::ColorStream &stream, const ForAll &forAll);
output::ColorStream &operator<<(output::ColorStream &stream, const Implies &implies);
output::ColorStream &operator<<(output::ColorStream &stream, const Not &not_);
output::ColorStream &operator<<(output::ColorStream &stream, const Or &or_);

output::ColorStream &operator<<(output::ColorStream &stream, const Formula &formula);
output::ColorStream &operator<<(output::ColorStream &stream, const Term &term);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Primitives
////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, BinaryOperation::Operator operator_)
{
	switch (operator_)
	{
		case BinaryOperation::Operator::Plus:
			return (stream << output::Operator("+"));
		case BinaryOperation::Operator::Minus:
			return (stream << output::Operator("-"));
		case BinaryOperation::Operator::Multiplication:
			return (stream << output::Operator("*"));
		case BinaryOperation::Operator::Division:
			return (stream << output::Operator("/"));
		case BinaryOperation::Operator::Modulo:
			return (stream << output::Operator("%"));
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const BinaryOperation &binaryOperation)
{
	return (stream << "(" << binaryOperation.left << " " << binaryOperation.operator_ << " " << binaryOperation.right << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Boolean &boolean)
{
	if (boolean.value)
		return (stream << output::Boolean("#true"));

	return (stream << output::Boolean("#false"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, Comparison::Operator operator_)
{
	switch (operator_)
	{
		case Comparison::Operator::GreaterThan:
			return (stream << output::Operator(">"));
		case Comparison::Operator::LessThan:
			return (stream << output::Operator("<"));
		case Comparison::Operator::LessEqual:
			return (stream << output::Operator("<="));
		case Comparison::Operator::GreaterEqual:
			return (stream << output::Operator(">="));
		case Comparison::Operator::NotEqual:
			return (stream << output::Operator("!="));
		case Comparison::Operator::Equal:
			return (stream << output::Operator("="));
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Comparison &comparison)
{
	return (stream << comparison.left << " " << comparison.operator_ << " " << comparison.right);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Constant &constant)
{
	return (stream << constant.name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Function &function)
{
	stream << function.name;

	if (function.arguments.empty())
		return stream;

	stream << "(";

	for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
	{
		if (i != function.arguments.cbegin())
			stream << ", ";

		stream << (*i);
	}

	if (function.name.empty() && function.arguments.size() == 1)
		stream << ",";

	return (stream << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const In &in)
{
	return (stream << in.element << " " << output::Keyword("in") << " " << in.set);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Integer &integer)
{
	return (stream << output::Number<int>(integer.value));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Interval &interval)
{
	return (stream << interval.from << ".." << interval.to);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Predicate &predicate)
{
	stream << predicate.name;

	if (predicate.arguments.empty())
		return stream;

	stream << "(";

	for (auto i = predicate.arguments.cbegin(); i != predicate.arguments.cend(); i++)
	{
		if (i != predicate.arguments.cbegin())
			stream << ", ";

		stream << (*i);
	}

	return (stream << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const SpecialInteger &specialInteger)
{
	switch (specialInteger.type)
	{
		case SpecialInteger::Type::Infimum:
			return (stream << output::Number<std::string>("#inf"));
		case SpecialInteger::Type::Supremum:
			return (stream << output::Number<std::string>("#sup"));
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const String &string)
{
	return (stream << output::String(string.text.c_str()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Variable &variable)
{
	assert(!variable.name.empty());

	if (variable.type == ast::Variable::Type::Reserved || !isReservedVariableName(variable.name.c_str()))
		return (stream << output::Variable(variable.name.c_str()));

	const auto variableName = std::string(UserVariablePrefix) + variable.name;

	return (stream << output::Variable(variableName.c_str()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Expressions
////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const And &and_)
{
	stream << "(";

	for (auto i = and_.arguments.cbegin(); i != and_.arguments.cend(); i++)
	{
		if (i != and_.arguments.cbegin())
			stream << " " << output::Keyword("and") << " ";

		stream << (*i);
	}

	return (stream << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Biconditional &biconditional)
{
	return (stream << "(" << biconditional.left << " <-> " << biconditional.right << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Exists &exists)
{
	stream << output::Keyword("exists") << " ";

	for (auto i = exists.variables.cbegin(); i != exists.variables.cend(); i++)
	{
		if (i != exists.variables.cbegin())
			stream << ", ";

		stream << (*i);
	}

	return (stream << " " << exists.argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const ForAll &forAll)
{
	stream << output::Keyword("forall") << " ";

	for (auto i = forAll.variables.cbegin(); i != forAll.variables.cend(); i++)
	{
		if (i != forAll.variables.cbegin())
			stream << ", ";

		stream << (*i);
	}

	return (stream << " " << forAll.argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Implies &implies)
{
	return (stream << "(" << implies.antecedent << " -> " << implies.consequent << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Not &not_)
{
	return (stream << output::Keyword("not ") << not_.argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Or &or_)
{
	stream << "(";

	for (auto i = or_.arguments.cbegin(); i != or_.arguments.cend(); i++)
	{
		if (i != or_.arguments.cbegin())
			stream << " " << output::Keyword("or") << " ";

		stream << (*i);
	}

	return (stream << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Variants
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Variant>
struct VariantPrintVisitor
{
	template<class T>
	output::ColorStream &visit(const T &x, output::ColorStream &stream)
	{
		return (stream << x);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Formula &formula)
{
	return formula.accept(VariantPrintVisitor<ast::Formula>(), stream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &operator<<(output::ColorStream &stream, const Term &term)
{
	return term.accept(VariantPrintVisitor<ast::Term>(), stream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
