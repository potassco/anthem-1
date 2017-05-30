#ifndef __ANTHEM__HEAD_H
#define __ANTHEM__HEAD_H

#include <algorithm>
#include <experimental/optional>

#include <anthem/AST.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Head
//
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Collect Head Terms
////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported", context);

		ruleContext.headTerms.reserve(ruleContext.headTerms.size() + function.arguments.size());

		for (const auto &argument : function.arguments)
			ruleContext.headTerms.emplace_back(&argument);
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Term &term, Context &context, RuleContext &)
	{
		throwErrorAtLocation(term.location, "term currently unsupported in this context, function expected", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &, Context &, RuleContext &)
	{
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, Context &context, RuleContext &ruleContext)
	{
		term.data.accept(TermCollectFunctionTermsVisitor(), term, context, ruleContext);
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Literal &literal, Context &context, RuleContext &)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: rename, because not only terms are collected anymore
struct HeadLiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, Context &context, RuleContext &ruleContext)
	{
		ruleContext.numberOfHeadLiterals = 1;

		literal.data.accept(LiteralCollectFunctionTermsVisitor(), literal, context, ruleContext);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &ruleContext)
	{
		ruleContext.numberOfHeadLiterals = disjunction.elements.size();

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

			conditionalLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionalLiteral.literal, context, ruleContext);
		}
	}

	void visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &ruleContext)
	{
		ruleContext.isChoiceRule = true;
		ruleContext.numberOfHeadLiterals = aggregate.elements.size();

		if (aggregate.left_guard || aggregate.right_guard)
			throwErrorAtLocation(headLiteral.location, "aggregates with left or right guards currently unsupported", context);

		for (const auto &conditionalLiteral : aggregate.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional literals in aggregates currently unsupported", context);

			conditionalLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionalLiteral.literal, context, ruleContext);
		}
	}

	template<class T>
	void visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &)
	{
		throwErrorAtLocation(headLiteral.location, "head literal currently unsupported in this context, expected literal, disjunction, or aggregate", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Translate Head
////////////////////////////////////////////////////////////////////////////////////////////////////

struct FunctionTermTranslateVisitor
{
	// TODO: check correctness
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext, size_t &headVariableIndex)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported", context);

		std::vector<ast::Term> arguments;
		arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			arguments.emplace_back(ast::Variable(ruleContext.freeVariables[headVariableIndex++].get()));

		return ast::Formula::make<ast::Predicate>(function.name, std::move(arguments));
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::Term &term, Context &context, RuleContext &, size_t &)
	{
		throwErrorAtLocation(term.location, "term currently unsupported in this context, function expected", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, Context &, RuleContext &, size_t &)
	{
		return ast::Formula::make<ast::Boolean>(boolean.value);
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, Context &context, RuleContext &ruleContext, size_t &headVariableIndex)
	{
		return term.data.accept(FunctionTermTranslateVisitor(), term, context, ruleContext, headVariableIndex);
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &literal, Context &context, RuleContext &, size_t &)
	{
		throwErrorAtLocation(literal.location, "only disjunctions of literals allowed as head literals", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralTranslateToConsequentVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, Context &context, RuleContext &ruleContext, size_t &headVariableIndex)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throwErrorAtLocation(literal.location, "double-negated head literals currently unsupported", context);

		auto translatedLiteral = literal.data.accept(LiteralTranslateVisitor(), literal, context, ruleContext, headVariableIndex);

		if (literal.sign == Clingo::AST::Sign::None)
			return translatedLiteral;

		if (!translatedLiteral)
			return std::experimental::nullopt;

		return ast::Formula::make<ast::Not>(std::move(translatedLiteral.value()));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &ruleContext, size_t &headVariableIndex)
	{
		std::vector<ast::Formula> arguments;
		arguments.reserve(disjunction.elements.size());

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

			auto argument = visit(conditionalLiteral.literal, headLiteral, context, ruleContext, headVariableIndex);

			if (!argument)
				throwErrorAtLocation(headLiteral.location, "could not parse argument", context);

			arguments.emplace_back(std::move(argument.value()));
		}

		return ast::Formula::make<ast::Or>(std::move(arguments));
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &ruleContext, size_t &headVariableIndex)
	{
		if (aggregate.left_guard || aggregate.right_guard)
			throwErrorAtLocation(headLiteral.location, "aggregates with left or right guards currently unsupported", context);

		const auto translateConditionalLiteral =
			[&](const auto &conditionalLiteral)
			{
				if (!conditionalLiteral.condition.empty())
					throwErrorAtLocation(headLiteral.location, "conditional head literals currently unsupported", context);

				return this->visit(conditionalLiteral.literal, headLiteral, context, ruleContext, headVariableIndex);
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

		return ast::Formula::make<ast::Or>(std::move(arguments));
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &, size_t &)
	{
		throwErrorAtLocation(headLiteral.location, "head literal currently unsupported in this context, expected literal, disjunction, or aggregate", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
