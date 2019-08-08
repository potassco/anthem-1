#ifndef __ANTHEM__VERIFY_STRONG_EQUIVALENCE__BODY_H
#define __ANTHEM__VERIFY_STRONG_EQUIVALENCE__BODY_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ComparisonOperator.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>
#include <anthem/verify-strong-equivalence/ChooseValueInTerm.h>

namespace anthem
{
namespace verifyStrongEquivalence
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Body
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyTermTranslateVisitor
{
	ast::Formula visit(const Clingo::AST::Function &function,
		const Clingo::AST::Literal &literal, const Clingo::AST::Term &, Context &context,
		RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
		predicateDeclaration->isUsed = true;

		ast::Predicate predicate(predicateDeclaration);

		// Create new body variable declarations
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(function.arguments.size());

		for (int i = 0; i < static_cast<int>(function.arguments.size()); i++)
		{
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
			// TODO: should be Domain::Unknown
			parameters.back()->domain = Domain::Symbolic;
			predicate.arguments.emplace_back(ast::Variable(parameters[i].get()));
		}

		ast::And and_;
		and_.arguments.reserve(parameters.size() + 1);

		for (int i = 0; i < static_cast<int>(function.arguments.size()); i++)
		{
			auto &argument = function.arguments[i];
			and_.arguments.emplace_back(chooseValueInTerm(argument, *parameters[i], context, ruleContext, variableStack));
		}

		auto makePredicateLiteral =
			[&]() -> ast::Formula
			{
				switch (literal.sign)
				{
					case Clingo::AST::Sign::None:
						return std::move(predicate);
					case Clingo::AST::Sign::Negation:
						return ast::Not(std::move(predicate));
					case Clingo::AST::Sign::DoubleNegation:
						return ast::Not(ast::Not(std::move(predicate)));
				}

				throw LogicException("unreachable code, please report to bug tracker");
			};

		auto predicateLiteral = makePredicateLiteral();
		and_.arguments.emplace_back(std::move(predicateLiteral));

		if (parameters.empty())
			return std::move(and_.arguments.front());

		return ast::Exists(std::move(parameters), std::move(and_));
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::Literal &,
		const Clingo::AST::Term &term, Context &, RuleContext &, ast::VariableStack &)
	{
		assert(!term.data.is<Clingo::AST::Function>());

		throw TranslationException(term.location, "term currently not yet supported in this context, expected function");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralTranslateVisitor
{
	ast::Formula visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &literal, Context &, RuleContext &, ast::VariableStack &)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated booleans not expected, please report to bug tracker");

		return ast::Boolean(boolean.value);
	}

	ast::Formula visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, Context &context, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		return term.data.accept(BodyTermTranslateVisitor(), literal, term, context, ruleContext, variableStack);
	}

	ast::Formula visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, Context &context, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated comparisons not expected, please report to bug tracker");

		ast::VariableDeclarationPointers parameters;
		parameters.reserve(2);

		for (size_t i = 0; i < 2; i++)
		{
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
			// TODO: should be Domain::Unknown
			parameters.back()->domain = Domain::Symbolic;
		}

		ast::And and_;
		and_.arguments.reserve(3);

		auto &parameterZ1 = parameters[0];
		auto &parameterZ2 = parameters[1];

		auto chooseZ1InT1 = chooseValueInTerm(comparison.left, *parameterZ1, context, ruleContext, variableStack);
		and_.arguments.emplace_back(std::move(chooseZ1InT1));

		auto chooseZ2InT2 = chooseValueInTerm(comparison.right, *parameterZ2, context, ruleContext, variableStack);
		and_.arguments.emplace_back(std::move(chooseZ2InT2));

		const auto operator_ = translate(comparison.comparison);
		auto compareZ1AndZ2 = ast::Comparison(operator_, ast::Variable(parameterZ1.get()), ast::Variable(parameterZ2.get()));
		and_.arguments.emplace_back(std::move(compareZ1AndZ2));

		return ast::Exists(std::move(parameters), std::move(and_));
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::Literal &literal, Context &, RuleContext &, ast::VariableStack &)
	{
		throw TranslationException(literal.location, "literal not yet supported in this context, expected boolean, comparison, or term");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyBodyLiteralTranslateVisitor
{
	ast::Formula visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		if (bodyLiteral.sign != Clingo::AST::Sign::None)
			throw TranslationException(bodyLiteral.location, "only positive body literals supported currently");

		// Negated literals require us to translate the rules to formulas in the logic of here-and-there
		if (literal.sign != Clingo::AST::Sign::None)
			context.semantics = Semantics::LogicOfHereAndThere;

		return literal.data.accept(BodyLiteralTranslateVisitor(), literal, context, ruleContext, variableStack);
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &, RuleContext &, ast::VariableStack &)
	{
		throw TranslationException(bodyLiteral.location, "body literal not yet supported in this context, expected literal");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
