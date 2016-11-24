#ifndef __ANTHEM__HEAD_H
#define __ANTHEM__HEAD_H

#include <algorithm>

#include <anthem/Utils.h>
#include <anthem/output/ClingoOutput.h>
#include <anthem/output/Formatting.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Head
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermCollectFunctionTermsVisitor
{
	void visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“symbol” terms not allowed, function expected", context);
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, Context &context)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported", context);

		context.headTerms.reserve(context.headTerms.size() + function.arguments.size());

		for (const auto &argument : function.arguments)
			context.headTerms.emplace_back(&argument);
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported, function expected", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &, Context &context)
	{
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, Context &context)
	{
		term.data.accept(TermCollectFunctionTermsVisitor(), term, context);
	}

	void visit(const Clingo::AST::Comparison &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, Context &context)
	{
		literal.data.accept(LiteralCollectFunctionTermsVisitor(), literal, context);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		for (const auto &conditionLiteral : disjunction.elements)
		{
			if (!conditionLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

			conditionLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionLiteral.literal, context);
		}
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“aggregate” head literals currently unsupported", context);
	}

	void visit(const Clingo::AST::HeadAggregate &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“head aggregate” head literals currently unsupported", context);
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“theory” head literals currently unsupported", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermPrintSubstitutedVisitor
{
	void visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“symbol” terms not allowed, function expected", context);
	}

	void visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported, function expected", context);
	}

	void visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported, function expected", context);
	}

	// TODO: check correctness
	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, Context &context)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported", context);

		auto &outputStream = context.logger.outputStream();

		outputStream << output::Function(function.name);

		if (function.arguments.empty())
			return;

		outputStream << "(";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				outputStream << ", ";

			const auto &argument = *i;

			const auto matchingTerm = std::find(context.headTerms.cbegin(), context.headTerms.cend(), &argument);

			assert(matchingTerm != context.headTerms.cend());

			const auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(matchingTerm - context.headTerms.cbegin() + 1);

			outputStream << output::Variable(variableName.c_str());
		}

		outputStream << ")";
	}

	void visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported, function expected", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralPrintSubstitutedVisitor
{
	void visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, Context &context)
	{
		context.logger.outputStream() << boolean;
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, Context &context)
	{
		term.data.accept(TermPrintSubstitutedVisitor(), term, context);
	}

	void visit(const Clingo::AST::Comparison &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
	}

	void visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralPrintSubstitutedVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, Context &context)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated literals currently unsupported", context);

		context.logger.outputStream() << literal.sign;

		literal.data.accept(LiteralPrintSubstitutedVisitor(), literal, context);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		for (auto i = disjunction.elements.cbegin(); i != disjunction.elements.cend(); i++)
		{
			const auto &conditionLiteral = *i;

			if (!conditionLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

			if (i != disjunction.elements.cbegin())
				context.logger.outputStream() << " " << Clingo::AST::BinaryOperator::Or << " ";

			visit(conditionLiteral.literal, headLiteral, context);
		}
	}

	void visit(const Clingo::AST::Aggregate &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“aggregate” head literals currently unsupported", context);
	}

	void visit(const Clingo::AST::HeadAggregate &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“head aggregate” head literals currently unsupported", context);
	}

	void visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“theory” head literals currently unsupported", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
