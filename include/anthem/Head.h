#ifndef __ANTHEM__HEAD_H
#define __ANTHEM__HEAD_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/Utils.h>
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
	void visit(const Clingo::Symbol &, const Clingo::AST::Term &term, Context &context)
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
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &, Context &)
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

// TODO: rename, because not only terms are collected anymore
struct HeadLiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, Context &context)
	{
		context.numberOfHeadLiterals = 1;

		literal.data.accept(LiteralCollectFunctionTermsVisitor(), literal, context);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		context.numberOfHeadLiterals = disjunction.elements.size();

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

			conditionalLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionalLiteral.literal, context);
		}
	}

	void visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		context.isChoiceRule = true;
		context.numberOfHeadLiterals = aggregate.elements.size();

		if (aggregate.left_guard || aggregate.right_guard)
			throwErrorAtLocation(headLiteral.location, "aggregates with left or right guards not allowed", context);

		for (const auto &conditionalLiteral : aggregate.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional literals in aggregates currently unsupported", context);

			conditionalLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionalLiteral.literal, context);
		}
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

struct FunctionTermTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::Symbol &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“symbol” terms not allowed, function expected", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Variable &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“variable” terms currently unsupported, function expected", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported, function expected", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::BinaryOperation &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported, function expected", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Interval &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“interval” terms currently unsupported, function expected", context);
		return std::experimental::nullopt;
	}

	// TODO: check correctness
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, Context &context)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported", context);

		std::vector<ast::Term> arguments;
		arguments.reserve(function.arguments.size());

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			const auto &argument = *i;
			const auto matchingTerm = std::find(context.headTerms.cbegin(), context.headTerms.cend(), &argument);

			assert(matchingTerm != context.headTerms.cend());

			auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(matchingTerm - context.headTerms.cbegin() + 1);
			arguments.emplace_back(std::make_unique<ast::Variable>(std::move(variableName), ast::Variable::Type::Reserved));
		}

		return std::make_unique<ast::Predicate>(function.name, std::move(arguments));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, Context &context)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported, function expected", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, Context &)
	{
		return std::make_unique<ast::Boolean>(boolean.value);
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, Context &context)
	{
		return term.data.accept(FunctionTermTranslateVisitor(), term, context);
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Comparison &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::CSPLiteral &, const Clingo::AST::Literal &literal, Context &context)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralTranslateToConsequentVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, Context &context)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated head literals currently unsupported", context);

		auto translatedLiteral = literal.data.accept(LiteralTranslateVisitor(), literal, context);

		if (literal.sign == Clingo::AST::Sign::None)
			return translatedLiteral;

		if (!translatedLiteral)
			return std::experimental::nullopt;

		return std::make_unique<ast::Not>(std::move(translatedLiteral.value()));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		std::vector<ast::Formula> arguments;
		arguments.reserve(disjunction.elements.size());

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

			auto argument = visit(conditionalLiteral.literal, headLiteral, context);

			if (!argument)
				throwErrorAtLocation(headLiteral.location, "could not parse argument", context);

			arguments.emplace_back(std::move(argument.value()));
		}

		return std::make_unique<ast::Or>(std::move(arguments));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		if (aggregate.left_guard || aggregate.right_guard)
			throwErrorAtLocation(headLiteral.location, "aggregates with left or right guards not allowed", context);

		const auto translateConditionalLiteral =
			[&](const auto &conditionalLiteral)
			{
				if (!conditionalLiteral.condition.empty())
					throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

				return this->visit(conditionalLiteral.literal, headLiteral, context);
			};

		if (aggregate.elements.size() == 1)
			return translateConditionalLiteral(aggregate.elements[0]);

		std::vector<ast::Formula> arguments;
		arguments.reserve(aggregate.elements.size());

		for (const auto &conditionalLiteral : aggregate.elements)
		{
			auto argument = translateConditionalLiteral(conditionalLiteral);

			if (!argument)
				throwErrorAtLocation(headLiteral.location, "could not parse argument", context);

			arguments.emplace_back(std::move(argument.value()));
		}

		return std::make_unique<ast::Or>(std::move(arguments));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::HeadAggregate &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“head aggregate” head literals currently unsupported", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::TheoryAtom &, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		throwErrorAtLocation(headLiteral.location, "“theory” head literals currently unsupported", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
