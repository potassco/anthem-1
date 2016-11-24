#include <anthem/output/ClingoOutput.h>

#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ClingoOutput
//
////////////////////////////////////////////////////////////////////////////////////////////////////

const auto printCollection = [](auto &stream, const auto &collection,
	const auto &preToken, const auto &delimiter, const auto &postToken, bool printTokensIfEmpty)
{
	if (collection.empty() && !printTokensIfEmpty)
		return;

	if (collection.empty() && printTokensIfEmpty)
	{
		stream << preToken << postToken;
		return;
	}

	stream << preToken;

	// TODO: use cbegin/cend (requires support by Clingo::SymbolSpan)
	for (auto i = collection.begin(); i != collection.end(); i++)
	{
		if (i != collection.begin())
			stream << delimiter;

		stream << *i;
	}

	stream << postToken;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::Symbol &symbol)
{
	switch (symbol.type())
	{
		case Clingo::SymbolType::Infimum:
			return (stream << Keyword("#inf"));
		case Clingo::SymbolType::Supremum:
			return (stream << Keyword("#sup"));
		case Clingo::SymbolType::Number:
			return (stream << Number<decltype(symbol.number())>(symbol.number()));
		case Clingo::SymbolType::String:
			return (stream << String(symbol.string()));
		case Clingo::SymbolType::Function:
		{
			const auto isNegative = symbol.is_negative();
			assert(isNegative != symbol.is_positive());

			const auto isUnaryTuple = (symbol.name()[0] == '\0' && symbol.arguments().size() == 1);
			const auto printIfEmpty = (symbol.name()[0] == '\0' || !symbol.arguments().empty());

			if (isNegative)
				stream << Operator("-");

			stream << Function(symbol.name());

			const auto postToken = (isUnaryTuple ? ",)" : ")");

			printCollection(stream, symbol.arguments(), "(", ", ", postToken, printIfEmpty);

			return stream;
		}
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Sign &sign)
{
	switch (sign)
	{
		case Clingo::AST::Sign::None:
			return stream;
		case Clingo::AST::Sign::Negation:
			return (stream << Keyword("not") << " ");
		case Clingo::AST::Sign::DoubleNegation:
			return (stream << Keyword("not") << " " << Keyword("not") << " ");
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Boolean &boolean)
{
	if (boolean.value == true)
		return (stream << Boolean("#true"));

	return (stream << Boolean("#false"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Variable &variable)
{
	if (!isReservedVariableName(variable.name))
		return (stream << Variable(variable.name));

	const auto variableName = std::string(UserVariablePrefix) + variable.name;

	return (stream << Variable(variableName.c_str()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::BinaryOperator &binaryOperator)
{
	switch (binaryOperator)
	{
		case Clingo::AST::BinaryOperator::XOr:
			return (stream << Keyword("xor"));
		case Clingo::AST::BinaryOperator::Or:
			return (stream << Keyword("or"));
		case Clingo::AST::BinaryOperator::And:
			return (stream << Keyword("and"));
		case Clingo::AST::BinaryOperator::Plus:
			return (stream << Operator("+"));
		case Clingo::AST::BinaryOperator::Minus:
			return (stream << Operator("-"));
		case Clingo::AST::BinaryOperator::Multiplication:
			return (stream << Operator("*"));
		case Clingo::AST::BinaryOperator::Division:
			return (stream << Operator("/"));
		case Clingo::AST::BinaryOperator::Modulo:
			return (stream << Operator("\\"));
	}

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::UnaryOperation &unaryOperation)
{
	return (stream
		<< Keyword(Clingo::AST::left_hand_side(unaryOperation.unary_operator))
		<< unaryOperation.argument
		<< Keyword(Clingo::AST::right_hand_side(unaryOperation.unary_operator)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::BinaryOperation &binaryOperation)
{
	return (stream << "(" << binaryOperation.left
		<< " " << binaryOperation.binary_operator
		<< " " << binaryOperation.right << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Interval &interval)
{
	return (stream << "(" << interval.left << Operator("..") << interval.right << ")");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Function &function)
{
	const auto isUnaryTuple = (function.name[0] == '\0' && function.arguments.size() == 1);
	const auto printIfEmpty = (function.name[0] == '\0' || !function.arguments.empty());

	if (function.external)
		stream << Operator("@");

	const auto postToken = (isUnaryTuple ? ",)" : ")");

	printCollection(stream, function.arguments, "(", ", ", postToken, printIfEmpty);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Pool &pool)
{
	// Note: There is no representation for an empty pool
	if (pool.arguments.empty())
		return (stream << "(" << Number<int>(1) << "/" << Number<int>(0) << ")");

	printCollection(stream, pool.arguments, "(", ";", ")", true);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermOutputVisitor
{
	void visit(const Clingo::Symbol &symbol, ColorStream &stream)
	{
		stream << symbol;
	}

	void visit(const Clingo::AST::Variable &variable, ColorStream &stream)
	{
		stream << variable;
	}

	void visit(const Clingo::AST::UnaryOperation &unaryOperation, ColorStream &stream)
	{
		stream << unaryOperation;
	}

	void visit(const Clingo::AST::BinaryOperation &binaryOperation, ColorStream &stream)
	{
		stream << binaryOperation;
	}

	void visit(const Clingo::AST::Interval &interval, ColorStream &stream)
	{
		stream << interval;
	}

	void visit(const Clingo::AST::Function &function, ColorStream &stream)
	{
		stream << function;
	}

	void visit(const Clingo::AST::Pool &pool, ColorStream &stream)
	{
		stream << pool;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Term &term)
{
	term.data.accept(TermOutputVisitor(), stream);

	return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
