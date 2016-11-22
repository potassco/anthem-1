#ifndef __ANTHEM__LITERAL_VISITOR_H
#define __ANTHEM__LITERAL_VISITOR_H

#include <anthem/TermVisitor.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LiteralVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void throwErrorUnsupportedLiteral(const char *statementType, const Clingo::AST::Literal &literal)
{
	const auto errorMessage = std::string("“") + statementType + "” literals currently not supported";

	throwErrorAtLocation(literal.location, errorMessage.c_str());

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralVisitor
{
	void visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &)
	{
		if (boolean.value == true)
			std::cout << "true";
		else
			std::cout << "false";
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &)
	{
		term.data.accept(TermVisitor(), term);
	}

	void visit(const Clingo::AST::Comparison &, const Clingo::AST::Literal &literal)
	{
		throwErrorUnsupportedLiteral("comparison", literal);
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal)
	{
		throwErrorUnsupportedLiteral("CSP literal", literal);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralCollectVariablesVisitor
{
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &, std::vector<Clingo::AST::Variable> &)
	{
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, std::vector<Clingo::AST::Variable> &variables)
	{
		term.data.accept(TermCollectVariablesVisitor(), term, variables);
	}

	void visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &, std::vector<Clingo::AST::Variable> &variables)
	{
		comparison.left.data.accept(TermCollectVariablesVisitor(), comparison.left, variables);
		comparison.right.data.accept(TermCollectVariablesVisitor(), comparison.right, variables);
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, std::vector<Clingo::AST::Variable> &)
	{
		throwErrorUnsupportedLiteral("CSP literal", literal);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
