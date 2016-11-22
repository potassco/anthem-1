#ifndef __ANTHEM__BODY_LITERAL_VISITOR_H
#define __ANTHEM__BODY_LITERAL_VISITOR_H

#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// BodyLiteralVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void throwErrorUnsupportedBodyLiteral(const char *statementType, const Clingo::AST::BodyLiteral &bodyLiteral)
{
	const auto errorMessage = std::string("“") + statementType + "” body literals currently not supported";

	throwErrorAtLocation(bodyLiteral.location, errorMessage.c_str());

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralVisitor
{
	void visit(const Clingo::AST::Literal &, const Clingo::AST::BodyLiteral &)
	{
		std::cout << "[literal]" << std::endl;
	}

	void visit(const Clingo::AST::ConditionalLiteral &, const Clingo::AST::BodyLiteral &bodyLiteral)
	{
		throwErrorUnsupportedBodyLiteral("conditional literal", bodyLiteral);
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::BodyLiteral &bodyLiteral)
	{
		throwErrorUnsupportedBodyLiteral("aggregate", bodyLiteral);
	}

	void visit(const Clingo::AST::BodyAggregate &, const Clingo::AST::BodyLiteral &bodyLiteral)
	{
		throwErrorUnsupportedBodyLiteral("body aggregate", bodyLiteral);
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::BodyLiteral &bodyLiteral)
	{
		throwErrorUnsupportedBodyLiteral("theory atom", bodyLiteral);
	}

	void visit(const Clingo::AST::Disjoint &, const Clingo::AST::BodyLiteral &bodyLiteral)
	{
		throwErrorUnsupportedBodyLiteral("disjoint", bodyLiteral);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
