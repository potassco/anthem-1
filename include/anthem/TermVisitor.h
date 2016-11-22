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

struct TermCollectVariablesVisitor
{
	void visit(const Clingo::Symbol &, const Clingo::AST::Term &, std::vector<Clingo::AST::Variable> &)
	{
	}

	void visit(const Clingo::AST::Variable &variable, const Clingo::AST::Term &, std::vector<Clingo::AST::Variable> &variables)
	{
		variables.push_back(variable);
	}

	void visit(const Clingo::AST::UnaryOperation &unaryOperation, const Clingo::AST::Term &, std::vector<Clingo::AST::Variable> &variables)
	{
		unaryOperation.argument.data.accept(*this, unaryOperation.argument, variables);
	}

	void visit(const Clingo::AST::BinaryOperation &binaryOperation, const Clingo::AST::Term &, std::vector<Clingo::AST::Variable> &variables)
	{
		binaryOperation.left.data.accept(*this, binaryOperation.left, variables);
		binaryOperation.right.data.accept(*this, binaryOperation.right, variables);
	}

	void visit(const Clingo::AST::Interval &interval, const Clingo::AST::Term &, std::vector<Clingo::AST::Variable> &variables)
	{
		interval.left.data.accept(*this, interval.left, variables);
		interval.right.data.accept(*this, interval.right, variables);
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, std::vector<Clingo::AST::Variable> &variables)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently not supported");

		for (const auto &argument : function.arguments)
			argument.data.accept(*this, argument, variables);
	}

	void visit(const Clingo::AST::Pool &pool, const Clingo::AST::Term &, std::vector<Clingo::AST::Variable> &variables)
	{
		for (const auto &argument : pool.arguments)
			argument.data.accept(*this, argument, variables);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
