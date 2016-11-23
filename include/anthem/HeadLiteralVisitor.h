#ifndef __ANTHEM__HEAD_LITERAL_VISITOR_H
#define __ANTHEM__HEAD_LITERAL_VISITOR_H

#include <anthem/LiteralVisitor.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HeadLiteralVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void throwErrorUnsupportedHeadLiteral(const char *statementType, const Clingo::AST::HeadLiteral &headLiteral)
{
	const auto errorMessage = std::string("“") + statementType + "” head literals currently unsupported";

	throwErrorAtLocation(headLiteral.location, errorMessage.c_str());

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throwErrorAtLocation(literal.location, "only positive literals currently supported");

		literal.data.accept(LiteralVisitor(), literal);
	}

	void visit(const Clingo::AST::Disjunction &, const Clingo::AST::HeadLiteral &headLiteral)
	{
		// TODO: implement
		throwErrorUnsupportedHeadLiteral("disjunction", headLiteral);
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::HeadLiteral &headLiteral)
	{
		throwErrorUnsupportedHeadLiteral("aggregate", headLiteral);
	}

	void visit(const Clingo::AST::HeadAggregate &, const Clingo::AST::HeadLiteral &headLiteral)
	{
		throwErrorUnsupportedHeadLiteral("head aggregate", headLiteral);
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::HeadLiteral &headLiteral)
	{
		throwErrorUnsupportedHeadLiteral("theory", headLiteral);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
