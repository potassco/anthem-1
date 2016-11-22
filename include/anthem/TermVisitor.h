#ifndef __ANTHEM__TERM_VISITOR_H
#define __ANTHEM__TERM_VISITOR_H

#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TermVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void throwErrorUnsupportedTerm(const char *statementType, const Clingo::AST::Term &term)
{
	const auto errorMessage = std::string("“") + statementType + "” terms currently not supported";

	throwErrorAtLocation(term.location, errorMessage.c_str());

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermVisitor
{
	void visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &)
	{
		std::cout << symbol;
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term)
	{
		throwErrorUnsupportedTerm("variable", term);
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term)
	{
		throwErrorUnsupportedTerm("unary operation", term);
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term)
	{
		throwErrorUnsupportedTerm("binary operation", term);
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term)
	{
		throwErrorUnsupportedTerm("interval", term);
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term)
	{
		std::cout << "[" << function.name << "]";

		throwErrorUnsupportedTerm("function", term);
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term)
	{
		throwErrorUnsupportedTerm("pool", term);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
