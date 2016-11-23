#ifndef __ANTHEM__HEAD_H
#define __ANTHEM__HEAD_H

#include <algorithm>

#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Head
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermCollectFunctionTermsVisitor
{
	void visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“symbol” terms not allowed, function expected");
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &terms)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported");

		terms.reserve(terms.size() + function.arguments.size());

		for (const auto &argument : function.arguments)
			terms.emplace_back(&argument);
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported, function expected");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &, std::vector<const Clingo::AST::Term *> &)
	{
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, std::vector<const Clingo::AST::Term *> &terms)
	{
		term.data.accept(TermCollectFunctionTermsVisitor(), term, terms);
	}

	void visit(const Clingo::AST::Comparison &, const Clingo::AST::Literal &literal, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals");
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, std::vector<const Clingo::AST::Term *> &terms)
	{
		literal.data.accept(LiteralCollectFunctionTermsVisitor(), literal, terms);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, std::vector<const Clingo::AST::Term *> &terms)
	{
		for (const auto &conditionLiteral : disjunction.elements)
		{
			if (!conditionLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported");

			conditionLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionLiteral.literal, terms);
		}
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::HeadLiteral &headLiteral, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(headLiteral.location, "“aggregate” head literals currently unsupported");
	}

	void visit(const Clingo::AST::HeadAggregate &, const Clingo::AST::HeadLiteral &headLiteral, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(headLiteral.location, "“head aggregate” head literals currently unsupported");
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::HeadLiteral &headLiteral, std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(headLiteral.location, "“theory” head literals currently unsupported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermPrintSubstitutedVisitor
{
	void visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“symbol” terms not allowed, function expected");
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported, function expected");
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &terms)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported");

		std::cout << function.name;

		if (function.arguments.empty())
			return;

		std::cout << "(";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				std::cout << ",";

			const auto &argument = *i;

			const auto matchingTerm = std::find(terms.cbegin(), terms.cend(), &argument);

			assert(matchingTerm != terms.cend());

			std::cout << AuxiliaryHeadVariablePrefix << (matchingTerm - terms.cbegin());
		}

		std::cout << ")";
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported, function expected");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralPrintSubstitutedVisitor
{
	void visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, const std::vector<const Clingo::AST::Term *> &)
	{
		std::cout << (boolean.value == true ? "true" : "false");
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, const std::vector<const Clingo::AST::Term *> &terms)
	{
		term.data.accept(TermPrintSubstitutedVisitor(), term, terms);
	}

	void visit(const Clingo::AST::Comparison &, const Clingo::AST::Literal &literal, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals");
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralPrintSubstitutedVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, const std::vector<const Clingo::AST::Term *> &terms)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated literals currently unsupported");
		else if (literal.sign == Clingo::AST::Sign::Negation)
			std::cout << "not ";

		literal.data.accept(LiteralPrintSubstitutedVisitor(), literal, terms);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, const std::vector<const Clingo::AST::Term *> &terms)
	{
		for (auto i = disjunction.elements.cbegin(); i != disjunction.elements.cend(); i++)
		{
			const auto &conditionLiteral = *i;

			if (!conditionLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported");

			if (i != disjunction.elements.cbegin())
				std::cout << " or ";

			visit(conditionLiteral.literal, headLiteral, terms);
		}
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::HeadLiteral &headLiteral, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(headLiteral.location, "“aggregate” head literals currently unsupported");
	}

	void visit(const Clingo::AST::HeadAggregate &, const Clingo::AST::HeadLiteral &headLiteral, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(headLiteral.location, "“head aggregate” head literals currently unsupported");
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::HeadLiteral &headLiteral, const std::vector<const Clingo::AST::Term *> &)
	{
		throwErrorAtLocation(headLiteral.location, "“theory” head literals currently unsupported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
