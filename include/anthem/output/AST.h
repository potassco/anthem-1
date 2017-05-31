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
	std::map<const VariableDeclaration *, size_t> userVariableIDs;
	std::map<const VariableDeclaration *, size_t> headVariableIDs;
	std::map<const VariableDeclaration *, size_t> bodyVariableIDs;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

output::ColorStream &print(output::ColorStream &stream, const BinaryOperation::Operator operator_, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const BinaryOperation &binaryOperation, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Boolean &boolean, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Comparison &comparison, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Constant &constant, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Function &function, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const In &in, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Integer &integer, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Interval &interval, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Predicate &predicate, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const SpecialInteger &specialInteger, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const String &string, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Variable &variable, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const VariableDeclaration &variableDeclaration, PrintContext &printContext);

output::ColorStream &print(output::ColorStream &stream, const And &and_, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Biconditional &biconditional, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Exists &exists, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const ForAll &forAll, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Implies &implies, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Not &not_, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Or &or_, PrintContext &printContext);

output::ColorStream &print(output::ColorStream &stream, const Formula &formula, PrintContext &printContext);
output::ColorStream &print(output::ColorStream &stream, const Term &term, PrintContext &printContext);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Primitives
////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, BinaryOperation::Operator operator_, PrintContext &)
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

inline output::ColorStream &print(output::ColorStream &stream, const BinaryOperation &binaryOperation, PrintContext &printContext)
{
	stream << "(";
	print(stream, binaryOperation.left, printContext);
	stream << " ";
	print(stream, binaryOperation.operator_, printContext);
	stream << " ";
	print(stream, binaryOperation.right, printContext);
	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Boolean &boolean, PrintContext &)
{
	if (boolean.value)
		return (stream << output::Boolean("#true"));

	return (stream << output::Boolean("#false"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, Comparison::Operator operator_, PrintContext &)
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

inline output::ColorStream &print(output::ColorStream &stream, const Comparison &comparison, PrintContext &printContext)
{
	print(stream, comparison.left, printContext);
	stream << " ";
	print(stream, comparison.operator_, printContext);
	stream << " ";
	print(stream, comparison.right, printContext);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Constant &constant, PrintContext &)
{
	return (stream << constant.name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Function &function, PrintContext &printContext)
{
	stream << function.name;

	if (function.arguments.empty())
		return stream;

	stream << "(";

	for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
	{
		if (i != function.arguments.cbegin())
			stream << ", ";

		print(stream, *i, printContext);
	}

	if (function.name.empty() && function.arguments.size() == 1)
		stream << ",";

	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const In &in, PrintContext &printContext)
{
	print(stream, in.element, printContext);
	stream << " " << output::Keyword("in") << " ";
	print(stream, in.set, printContext);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Integer &integer, PrintContext &)
{
	return (stream << output::Number<int>(integer.value));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Interval &interval, PrintContext &printContext)
{
	print(stream, interval.from, printContext);
	stream << "..";
	print(stream, interval.to, printContext);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Predicate &predicate, PrintContext &printContext)
{
	stream << predicate.name;

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

inline output::ColorStream &print(output::ColorStream &stream, const SpecialInteger &specialInteger, PrintContext &)
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

inline output::ColorStream &print(output::ColorStream &stream, const String &string, PrintContext &)
{
	return (stream << output::String(string.text.c_str()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Variable &variable, PrintContext &printContext)
{
	assert(variable.declaration != nullptr);

	return print(stream, *variable.declaration, printContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const VariableDeclaration &variableDeclaration, PrintContext &printContext)
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

inline output::ColorStream &print(output::ColorStream &stream, const And &and_, PrintContext &printContext)
{
	stream << "(";

	for (auto i = and_.arguments.cbegin(); i != and_.arguments.cend(); i++)
	{
		if (i != and_.arguments.cbegin())
			stream << " " << output::Keyword("and") << " ";

		print(stream, *i, printContext);
	}

	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Biconditional &biconditional, PrintContext &printContext)
{
	stream << "(";
	print(stream, biconditional.left, printContext);
	stream << " <-> ";
	print(stream, biconditional.right, printContext);
	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Exists &exists, PrintContext &printContext)
{
	stream << output::Keyword("exists") << " ";

	for (auto i = exists.variables.cbegin(); i != exists.variables.cend(); i++)
	{
		if (i != exists.variables.cbegin())
			stream << ", ";

		print(stream, **i, printContext);
	}

	stream << " ";
	print(stream, exists.argument, printContext);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const ForAll &forAll, PrintContext &printContext)
{
	stream << output::Keyword("forall") << " ";

	for (auto i = forAll.variables.cbegin(); i != forAll.variables.cend(); i++)
	{
		if (i != forAll.variables.cbegin())
			stream << ", ";

		print(stream, **i, printContext);
	}

	stream << " ";
	print(stream, forAll.argument, printContext);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Implies &implies, PrintContext &printContext)
{
	stream << "(";
	print(stream, implies.antecedent, printContext);
	stream << " -> ";
	print(stream, implies.consequent, printContext);
	stream << ")";

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Not &not_, PrintContext &printContext)
{
	stream << output::Keyword("not") << " ";
	print(stream, not_.argument, printContext);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Or &or_, PrintContext &printContext)
{
	stream << "(";

	for (auto i = or_.arguments.cbegin(); i != or_.arguments.cend(); i++)
	{
		if (i != or_.arguments.cbegin())
			stream << " " << output::Keyword("or") << " ";

		print(stream, *i, printContext);
	}

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
	output::ColorStream &visit(const T &x, output::ColorStream &stream, PrintContext &printContext)
	{
		return print(stream, x, printContext);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Formula &formula, PrintContext &printContext)
{
	return formula.accept(VariantPrintVisitor<Formula>(), stream, printContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline output::ColorStream &print(output::ColorStream &stream, const Term &term, PrintContext &printContext)
{
	return term.accept(VariantPrintVisitor<Term>(), stream, printContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
