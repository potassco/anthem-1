#ifndef __ANTHEM__HEAD_H
#define __ANTHEM__HEAD_H

#include <algorithm>
#include <optional>

#include <anthem/AST.h>
#include <anthem/Exception.h>
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
	void visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, RuleContext &ruleContext)
	{
		if (function.external)
			throw LogicException(term.location, "external functions currently unsupported");

		ruleContext.headTerms.reserve(ruleContext.headTerms.size() + function.arguments.size());

		for (const auto &argument : function.arguments)
			ruleContext.headTerms.emplace_back(&argument);
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Term &term, RuleContext &)
	{
		throw LogicException(term.location, "term currently unsupported in this context, function expected");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Boolean &, const Clingo::AST::Literal &, RuleContext &)
	{
	}

	void visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, RuleContext &ruleContext)
	{
		term.data.accept(TermCollectFunctionTermsVisitor(), term, ruleContext);
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Literal &literal, RuleContext &)
	{
		throw LogicException(literal.location, "only disjunctions of literals allowed as head literals");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: rename, because not only terms are collected anymore
struct HeadLiteralCollectFunctionTermsVisitor
{
	void visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, RuleContext &ruleContext)
	{
		ruleContext.numberOfHeadLiterals = 1;

		literal.data.accept(LiteralCollectFunctionTermsVisitor(), literal, ruleContext);
	}

	void visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &ruleContext)
	{
		ruleContext.numberOfHeadLiterals = disjunction.elements.size();

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throw LogicException(headLiteral.location, "conditional head literals currently unsupported");

			conditionalLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionalLiteral.literal, ruleContext);
		}
	}

	void visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &ruleContext)
	{
		ruleContext.isChoiceRule = true;
		ruleContext.numberOfHeadLiterals = aggregate.elements.size();

		if (aggregate.left_guard || aggregate.right_guard)
			throw LogicException(headLiteral.location, "aggregates with left or right guards currently unsupported");

		for (const auto &conditionalLiteral : aggregate.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throw LogicException(headLiteral.location, "conditional literals in aggregates currently unsupported");

			conditionalLiteral.literal.data.accept(LiteralCollectFunctionTermsVisitor(), conditionalLiteral.literal, ruleContext);
		}
	}

	template<class T>
	void visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &)
	{
		throw LogicException(headLiteral.location, "head literal currently unsupported in this context, expected literal, disjunction, or aggregate");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Translate Head
////////////////////////////////////////////////////////////////////////////////////////////////////

struct FunctionTermTranslateVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, size_t &headVariableIndex)
	{
		if (function.external)
			throw TranslationException(term.location, "external functions currently unsupported");

		std::vector<ast::Term> arguments;
		arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			arguments.emplace_back(ast::Variable(ruleContext.freeVariables[headVariableIndex++].get()));

		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
		predicateDeclaration->isUsed = true;

		return ast::Predicate(predicateDeclaration, std::move(arguments));
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::Term &term, RuleContext &, Context &, size_t &)
	{
		throw TranslationException(term.location, "term currently unsupported in this context, function expected");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct LiteralTranslateVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, RuleContext &, Context &, size_t &)
	{
		return ast::Formula::make<ast::Boolean>(boolean.value);
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &, RuleContext &ruleContext, Context &context, size_t &headVariableIndex)
	{
		return term.data.accept(FunctionTermTranslateVisitor(), term, ruleContext, context, headVariableIndex);
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &literal, RuleContext &, Context &, size_t &)
	{
		throw TranslationException(literal.location, "only disjunctions of literals allowed as head literals");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralTranslateToConsequentVisitor
{
	std::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &, RuleContext &ruleContext, Context &context, size_t &headVariableIndex)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throw TranslationException(literal.location, "double-negated head literals currently unsupported");

		auto translatedLiteral = literal.data.accept(LiteralTranslateVisitor(), literal, ruleContext, context, headVariableIndex);

		if (literal.sign == Clingo::AST::Sign::None)
			return translatedLiteral;

		if (!translatedLiteral)
			return std::nullopt;

		return ast::Formula::make<ast::Not>(std::move(translatedLiteral.value()));
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Disjunction &disjunction, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &ruleContext, Context &context, size_t &headVariableIndex)
	{
		std::vector<ast::Formula> arguments;
		arguments.reserve(disjunction.elements.size());

		for (const auto &conditionalLiteral : disjunction.elements)
		{
			if (!conditionalLiteral.condition.empty())
				throw TranslationException(headLiteral.location, "conditional head literals currently unsupported");

			auto argument = visit(conditionalLiteral.literal, headLiteral, ruleContext, context, headVariableIndex);

			if (!argument)
				throw TranslationException(headLiteral.location, "could not parse argument");

			arguments.emplace_back(std::move(argument.value()));
		}

		return ast::Formula::make<ast::Or>(std::move(arguments));
	}

	std::optional<ast::Formula> visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &ruleContext, Context &context, size_t &headVariableIndex)
	{
		if (aggregate.left_guard || aggregate.right_guard)
			throw TranslationException(headLiteral.location, "aggregates with left or right guards currently unsupported");

		const auto translateConditionalLiteral =
			[&](const auto &conditionalLiteral)
			{
				if (!conditionalLiteral.condition.empty())
					throw TranslationException(headLiteral.location, "conditional head literals currently unsupported");

				return this->visit(conditionalLiteral.literal, headLiteral, ruleContext, context, headVariableIndex);
			};

		if (aggregate.elements.size() == 1)
			return translateConditionalLiteral(aggregate.elements[0]);

		std::vector<ast::Formula> arguments;
		arguments.reserve(aggregate.elements.size());

		for (const auto &conditionalLiteral : aggregate.elements)
		{
			auto argument = translateConditionalLiteral(conditionalLiteral);

			if (!argument)
				throw TranslationException(headLiteral.location, "could not parse argument");

			arguments.emplace_back(std::move(argument.value()));
		}

		return ast::Formula::make<ast::Or>(std::move(arguments));
	}

	template<class T>
	std::optional<ast::Formula> visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, RuleContext &, Context &, size_t &)
	{
		throw TranslationException(headLiteral.location, "head literal currently unsupported in this context, expected literal, disjunction, or aggregate");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
