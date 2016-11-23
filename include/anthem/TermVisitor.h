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

struct TermVisitor
{
	void visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &)
	{
		std::cout << symbol;
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported");
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported");
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported");
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported");
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term)
	{
		std::cout << "[" << function.name << "]";

		throwErrorAtLocation(term.location, "“function” terms currently unsupported");
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
