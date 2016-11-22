#ifndef __ANTHEM__HEAD_LITERAL_VISITOR_H
#define __ANTHEM__HEAD_LITERAL_VISITOR_H

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
	const auto errorMessage = std::string("“") + statementType + "” head literals currently not supported";

	throwErrorAtLocation(headLiteral.location, errorMessage.c_str());

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralVisitor
{
	void visit(const Clingo::AST::Literal &, const Clingo::AST::HeadLiteral &)
	{
		std::cout << "[literal]" << std::endl;
	}

	void visit(const Clingo::AST::Disjunction &, const Clingo::AST::HeadLiteral &headLiteral)
	{
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
