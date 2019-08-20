#ifndef __ANTHEM__VERIFY_PROPERTIES__BODY_H
#define __ANTHEM__VERIFY_PROPERTIES__BODY_H

#include <anthem/AST.h>
#include <anthem/translation-common/ChooseValueInTerm.h>
#include <anthem/translation-common/ComparisonOperator.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Body
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyTermVisitor
{
	ast::Formula visit(const Clingo::AST::Function &function,
		const Clingo::AST::Literal &literal, const Clingo::AST::Term &, Context &context,
		ast::VariableDeclarationPointers &freeVariables, ast::VariableStack &variableStack)
	{
		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
		predicateDeclaration->isUsed = true;

		ast::Predicate predicate(predicateDeclaration);

		// Create new body variable declarations
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(function.arguments.size());

		for (auto i = 0; i < static_cast<int>(function.arguments.size()); i++)
		{
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
			parameters.back()->domain = Domain::Program;
			predicate.arguments.emplace_back(ast::Variable(parameters[i].get()));
		}

		ast::And and_;
		and_.arguments.reserve(parameters.size() + 1);

		for (auto i = 0; i < static_cast<int>(function.arguments.size()); i++)
		{
			auto &argument = function.arguments[i];
			and_.arguments.emplace_back(translationCommon::chooseValueInTerm(argument, *parameters[i], context, freeVariables, variableStack));
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
						return std::move(predicate);
					default:
						throw LogicException("unexpected literal sign, please report to bug tracker");
				}
			};

		auto predicateLiteral = makePredicateLiteral();
		and_.arguments.emplace_back(std::move(predicateLiteral));

		if (parameters.empty())
			return std::move(and_.arguments.front());

		return ast::Exists(std::move(parameters), std::move(and_));
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::Literal &,
		const Clingo::AST::Term &term, Context &, ast::VariableDeclarationPointers &, ast::VariableStack &)
	{
		assert(!term.data.is<Clingo::AST::Function>());

		throw TranslationException(term.location, "term not yet supported, expected function");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralVisitor
{
	ast::Formula visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &literal, Context &, ast::VariableDeclarationPointers &, ast::VariableStack &)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw LogicException(literal.location, "unexpected negated boolean, please report to bug tracker");

		return ast::Boolean(boolean.value);
	}

	ast::Formula visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, Context &context, ast::VariableDeclarationPointers &freeVariables, ast::VariableStack &variableStack)
	{
		return term.data.accept(BodyTermVisitor(), literal, term, context, freeVariables, variableStack);
	}

	ast::Formula visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, Context &context, ast::VariableDeclarationPointers &freeVariables, ast::VariableStack &variableStack)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw LogicException(literal.location, "unexpected negated comparison, please report to bug tracker");

		ast::VariableDeclarationPointers parameters;
		parameters.reserve(2);

		for (auto i = 0; i < 2; i++)
		{
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
			parameters.back()->domain = Domain::Program;
		}

		ast::And and_;
		and_.arguments.reserve(3);

		auto &parameterZ1 = parameters[0];
		auto &parameterZ2 = parameters[1];

		auto chooseZ1InT1 = translationCommon::chooseValueInTerm(comparison.left, *parameterZ1, context, freeVariables, variableStack);
		and_.arguments.emplace_back(std::move(chooseZ1InT1));

		auto chooseZ2InT2 = translationCommon::chooseValueInTerm(comparison.right, *parameterZ2, context, freeVariables, variableStack);
		and_.arguments.emplace_back(std::move(chooseZ2InT2));

		const auto operator_ = translationCommon::translate(comparison.comparison);
		auto compareZ1AndZ2 = ast::Comparison(operator_, ast::Variable(parameterZ1.get()), ast::Variable(parameterZ2.get()));
		and_.arguments.emplace_back(std::move(compareZ1AndZ2));

		return ast::Exists(std::move(parameters), std::move(and_));
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::Literal &literal, Context &, ast::VariableDeclarationPointers &, ast::VariableStack &)
	{
		throw TranslationException(literal.location, "literal not yet supported, expected boolean, comparison, or term");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyBodyLiteralVisitor
{
	ast::Formula visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &bodyLiteral, Context &context, ast::VariableDeclarationPointers &freeVariables, ast::VariableStack &variableStack)
	{
		if (bodyLiteral.sign != Clingo::AST::Sign::None)
			throw TranslationException(bodyLiteral.location, "signed body literals not yet supported");

		return literal.data.accept(BodyLiteralVisitor(), literal, context, freeVariables, variableStack);
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::BodyLiteral &bodyLiteral, Context &, ast::VariableDeclarationPointers &, ast::VariableStack &)
	{
		throw TranslationException(bodyLiteral.location, "body literal not yet supported, expected literal");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
