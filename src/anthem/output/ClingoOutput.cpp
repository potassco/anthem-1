#include <anthem/output/ClingoOutput.h>

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
			return (stream << Keyword("sup"));
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

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Variable &variable)
{
	return (stream << Variable(variable.name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::AST::BinaryOperator &binaryOperator)
{
	switch (binaryOperator)
	{
		case Clingo::AST::BinaryOperator::XOr:
			return (stream << Operator("xor"));
		case Clingo::AST::BinaryOperator::Or:
			return (stream << Operator("or"));
		case Clingo::AST::BinaryOperator::And:
			return (stream << Operator("and"));
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
		return (stream << "(1/0)");

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

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////


/*
inline std::ostream &operator<<(std::ostream &out, Disjunction const &x) {
	out << Detail::print(x.elements, "", "; ", "", false);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, HeadAggregate const &x) {
	if (x.left_guard) { out << x.left_guard->term << " " << x.left_guard->comparison << " "; }
	out << x.function << " { " << Detail::print(x.elements, "", "; ", "", false) << " }";
	if (x.right_guard) { out << " " << x.right_guard->comparison << " " << x.right_guard->term; }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, HeadAggregateElement const &x) {
	out << Detail::print(x.tuple, "", ",", "", false) << " : " << x.condition;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, BodyAggregate const &x) {
	if (x.left_guard) { out << x.left_guard->term << " " << x.left_guard->comparison << " "; }
	out << x.function << " { " << Detail::print(x.elements, "", "; ", "", false) << " }";
	if (x.right_guard) { out << " " << x.right_guard->comparison << " " << x.right_guard->term; }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, BodyAggregateElement const &x) {
	out << Detail::print(x.tuple, "", ",", "", false) << " : " << Detail::print(x.condition, "", ", ", "", false);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Aggregate const &x) {
	if (x.left_guard) { out << x.left_guard->term << " " << x.left_guard->comparison << " "; }
	out << "{ " << Detail::print(x.elements, "", "; ", "", false) << " }";
	if (x.right_guard) { out << " " << x.right_guard->comparison << " " << x.right_guard->term; }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, ConditionalLiteral const &x) {
	out << x.literal << Detail::print(x.condition, " : ", ", ", "", true);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Literal const &x) {
	out << x.sign << x.data;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Boolean const &x) {
	out << (x.value ? "#true" : "#false");
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Comparison const &x) {
	out << x.left << x.comparison << x.right;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Id const &x) {
	out << x.id;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPLiteral const &x) {
	out << x.term;
	for (auto &y : x.guards) { out << y; }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPGuard const &x) {
	out << "$" << x.comparison << x.term;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPSum const &x) {
	if (x.terms.empty()) { out << "0"; }
	else				 { out << Detail::print(x.terms, "", "$+", "", false); }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPProduct const &x) {
	if (x.variable) { out << x.coefficient << "$*$" << *x.variable.get(); }
	else			{ out << x.coefficient; }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Pool const &x) {
	// NOTE: there is no representation for an empty pool
	if (x.arguments.empty()) { out << "(1/0)"; }
	else					 { out << Detail::print(x.arguments, "(", ";", ")", true); }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Function const &x) {
	bool tc = x.name[0] == '\0' && x.arguments.size() == 1;
	bool ey = x.name[0] == '\0' || !x.arguments.empty();
	out << (x.external ? "@" : "") << x.name << Detail::print(x.arguments, "(", ",", tc ? ",)" : ")", ey);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Interval const &x) {
	out << "(" << x.left << ".." << x.right << ")";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, BinaryOperation const &x) {
	out << "(" << x.left << x.binary_operator << x.right << ")";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, UnaryOperation const &x) {
	out << left_hand_side(x.unary_operator) << x.argument << right_hand_side(x.unary_operator);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Variable const &x) {
	out << x.name;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Term const &x) {
	out << x.data;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Rule const &x) {
	out << x.head << Detail::print_body(x.body, " :- ");
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Definition const &x) {
	out << "#const " << x.name << " = " << x.value << ".";
	if (x.is_default) { out << " [default]"; }
	return out;
}

inline std::ostream &operator<<(std::ostream &out, ShowSignature const &x) {
	out << "#show " << (x.csp ? "$" : "") << x.signature << ".";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, ShowTerm const &x) {
	out << "#show " << (x.csp ? "$" : "") << x.term << Detail::print_body(x.body);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Minimize const &x) {
	out << Detail::print_body(x.body, ":~ ") << " [" << x.weight << "@" << x.priority << Detail::print(x.tuple, ",", ",", "", false) << "]";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Script const &x) {
	std::string s = x.code;
	if (!s.empty() && s.back() == '\n') {
		s.back() = '.';
	}
	out << s;
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Program const &x) {
	out << "#program " << x.name << Detail::print(x.parameters, "(", ",", ")", false) << ".";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, External const &x) {
	out << "#external " << x.atom << Detail::print_body(x.body);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Edge const &x) {
	out << "#edge (" << x.u << "," << x.v << ")" << Detail::print_body(x.body);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Heuristic const &x) {
	out << "#heuristic " << x.atom << Detail::print_body(x.body) << " [" << x.bias<< "@" << x.priority << "," << x.modifier << "]";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, ProjectAtom const &x) {
	out << "#project " << x.atom << Detail::print_body(x.body);
	return out;
}

inline std::ostream &operator<<(std::ostream &out, ProjectSignature const &x) {
	out << "#project " << x.signature << ".";
	return out;
}

inline std::ostream &operator<<(std::ostream &out, Statement const &x) {
	out << x.data;
	return out;
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
