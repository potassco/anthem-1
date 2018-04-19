#ifndef __ANTHEM__OUTPUT__AST_H
#define __ANTHEM__OUTPUT__AST_H

#include <cassert>
#include <map>

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

struct PrintContext
{
	PrintContext(const Context &context)
	:	context{context}
	{
	}

	PrintContext(const PrintContext &other) = delete;
	PrintContext &operator=(const PrintContext &other) = delete;
	PrintContext(PrintContext &&other) = delete;
	PrintContext &operator=(PrintContext &&other) = delete;

	std::map<const VariableDeclaration *, size_t> userVariableIDs;
	std::map<const VariableDeclaration *, size_t> headVariableIDs;
	std::map<const VariableDeclaration *, size_t> bodyVariableIDs;

	const Context &context;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

output::ColorStream &print(output::ColorStream &stream, const BinaryOperation::Operator operator_, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const BinaryOperation &binaryOperation, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Boolean &boolean, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Comparison &comparison, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, Comparison::Operator operator_, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Function &function, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const In &in, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Integer &integer, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Interval &interval, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Predicate &predicate, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const PredicateDeclaration &predicateDeclaration, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const SpecialInteger &specialInteger, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const String &string, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const UnaryOperation &unaryOperation, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Variable &variable, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const VariableDeclaration &variableDeclaration, PrintContext &printContext, bool omitParentheses = false);

output::ColorStream &print(output::ColorStream &stream, const And &and_, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Biconditional &biconditional, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Exists &exists, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const ForAll &forAll, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Implies &implies, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Not &not_, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Or &or_, PrintContext &printContext, bool omitParentheses = false);

output::ColorStream &print(output::ColorStream &stream, const Formula &formula, PrintContext &printContext, bool omitParentheses = false);
output::ColorStream &print(output::ColorStream &stream, const Term &term, PrintContext &printContext, bool omitParentheses = false);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Primitives
////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, BinaryOperation::Operator operator_, PrintContext &, bool)
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
		case BinaryOperation::Operator::Power:
			return (stream << output::Operator("**"));
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const BinaryOperation &binaryOperation, PrintContext &printContext, bool omitParentheses)
{
	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	print(stream, binaryOperation.left, printContext);
	stream << " ";
	print(stream, binaryOperation.operator_, printContext);
	stream << " ";
	print(stream, binaryOperation.right, printContext);

	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Boolean &boolean, PrintContext &, bool)
{
	if (boolean.value)
		return (stream << output::Boolean("#true"));

	return (stream << output::Boolean("#false"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, Comparison::Operator operator_, PrintContext &, bool)
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

inline output::ColorStream &print(output::ColorStream &stream, const Comparison &comparison, PrintContext &printContext, bool)
{
	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	print(stream, comparison.left, printContext);
	stream << " ";
	print(stream, comparison.operator_, printContext);
	stream << " ";
	print(stream, comparison.right, printContext);

	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Function &function, PrintContext &printContext, bool)
{
	stream << function.declaration->name;

	if (function.arguments.empty())
		return stream;

	stream << "(";

	for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
	{
		if (i != function.arguments.cbegin())
			stream << ", ";

		print(stream, *i, printContext);
	}

	if (function.declaration->name.empty() && function.arguments.size() == 1)
		stream << ",";

	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const In &in, PrintContext &printContext, bool)
{
	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	print(stream, in.element, printContext);
	stream << " " << output::Keyword("in") << " ";
	print(stream, in.set, printContext);

	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Integer &integer, PrintContext &, bool)
{
	return (stream << output::Number<int>(integer.value));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Interval &interval, PrintContext &printContext, bool)
{
	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	print(stream, interval.from, printContext);
	stream << "..";
	print(stream, interval.to, printContext);

	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Predicate &predicate, PrintContext &printContext, bool)
{
	stream << predicate.declaration->name;

	if (predicate.arguments.empty())
		return stream;

	stream << "(";

	for (auto i = predicate.arguments.cbegin(); i != predicate.arguments.cend(); i++)
	{
		if (i != predicate.arguments.cbegin())
			stream << ", ";

		print(stream, *i, printContext);
	}

	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const PredicateDeclaration &predicateDeclaration, PrintContext &, bool)
{
	return (stream << predicateDeclaration.name << "/" << predicateDeclaration.arity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const SpecialInteger &specialInteger, PrintContext &, bool)
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

inline output::ColorStream &print(output::ColorStream &stream, const String &string, PrintContext &, bool)
{
	return (stream << output::String(string.text.c_str()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const UnaryOperation &unaryOperation, PrintContext &printContext, bool)
{
	switch (unaryOperation.operator_)
	{
		case UnaryOperation::Operator::Absolute:
			stream << "|";
			break;
	}

	print(stream, unaryOperation.argument, printContext, true);

	switch (unaryOperation.operator_)
	{
		case UnaryOperation::Operator::Absolute:
			stream << "|";
			break;
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Variable &variable, PrintContext &printContext, bool)
{
	assert(variable.declaration != nullptr);

	return print(stream, *variable.declaration, printContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const VariableDeclaration &variableDeclaration, PrintContext &printContext, bool)
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

	switch (variableDeclaration.type)
	{
		case VariableDeclaration::Type::UserDefined:
			return printVariableDeclaration(UserVariablePrefix, printContext.userVariableIDs);
		case VariableDeclaration::Type::Head:
			return printVariableDeclaration(HeadVariablePrefix, printContext.headVariableIDs);
		case VariableDeclaration::Type::Body:
			return printVariableDeclaration(BodyVariablePrefix, printContext.bodyVariableIDs);
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Expressions
////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const And &and_, PrintContext &printContext, bool omitParentheses)
{
	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	for (auto i = and_.arguments.cbegin(); i != and_.arguments.cend(); i++)
	{
		if (i != and_.arguments.cbegin())
			stream << " " << output::Keyword("and") << " ";

		print(stream, *i, printContext);
	}

	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Biconditional &biconditional, PrintContext &printContext, bool omitParentheses)
{
	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	print(stream, biconditional.left, printContext);
	stream << " <-> ";
	print(stream, biconditional.right, printContext);

	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Exists &exists, PrintContext &printContext, bool)
{
	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	stream << output::Keyword("exists") << " ";

	for (auto i = exists.variables.cbegin(); i != exists.variables.cend(); i++)
	{
		if (i != exists.variables.cbegin())
			stream << ", ";

		print(stream, **i, printContext);
	}

	stream << " ";
	print(stream, exists.argument, printContext);

	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const ForAll &forAll, PrintContext &printContext, bool)
{
	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	stream << output::Keyword("forall") << " ";

	for (auto i = forAll.variables.cbegin(); i != forAll.variables.cend(); i++)
	{
		if (i != forAll.variables.cbegin())
			stream << ", ";

		print(stream, **i, printContext);
	}

	stream << " ";
	print(stream, forAll.argument, printContext);

	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Implies &implies, PrintContext &printContext, bool omitParentheses)
{
	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	print(stream, implies.antecedent, printContext);
	stream << " -> ";
	print(stream, implies.consequent, printContext);

	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Not &not_, PrintContext &printContext, bool)
{
	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	stream << output::Keyword("not") << " ";
	print(stream, not_.argument, printContext);

	if (printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Or &or_, PrintContext &printContext, bool omitParentheses)
{
	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << "(";

	for (auto i = or_.arguments.cbegin(); i != or_.arguments.cend(); i++)
	{
		if (i != or_.arguments.cbegin())
			stream << " " << output::Keyword("or") << " ";

		print(stream, *i, printContext);
	}

	if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
		stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Variants
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Variant>
struct VariantPrintVisitor
{
	template<class T>
	output::ColorStream &visit(const T &x, output::ColorStream &stream, PrintContext &printContext, bool omitParentheses)
	{
		return print(stream, x, printContext, omitParentheses);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Formula &formula, PrintContext &printContext, bool omitParentheses)
{
	return formula.accept(VariantPrintVisitor<Formula>(), stream, printContext, omitParentheses);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Term &term, PrintContext &printContext, bool omitParentheses)
{
	return term.accept(VariantPrintVisitor<Term>(), stream, printContext, omitParentheses);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
