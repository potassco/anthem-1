#ifndef __ANTHEM__TERM_H
#define __ANTHEM__TERM_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Term
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ast::BinaryOperation::Operator translate(Clingo::AST::BinaryOperator binaryOperator, const Clingo::AST::Term &term)
{
	switch (binaryOperator)
	{
		case Clingo::AST::BinaryOperator::XOr:
			throw TranslationException(term.location, "binary operation “xor” currently unsupported");
		case Clingo::AST::BinaryOperator::Or:
			throw TranslationException(term.location, "binary operation “or” currently unsupported");
		case Clingo::AST::BinaryOperator::And:
			throw TranslationException(term.location, "binary operation “and” currently unsupported");
		case Clingo::AST::BinaryOperator::Plus:
			return ast::BinaryOperation::Operator::Plus;
		case Clingo::AST::BinaryOperator::Minus:
			return ast::BinaryOperation::Operator::Minus;
		case Clingo::AST::BinaryOperator::Multiplication:
			return ast::BinaryOperation::Operator::Multiplication;
		case Clingo::AST::BinaryOperator::Division:
			return ast::BinaryOperation::Operator::Division;
		case Clingo::AST::BinaryOperator::Modulo:
			return ast::BinaryOperation::Operator::Modulo;
		case Clingo::AST::BinaryOperator::Power:
			return ast::BinaryOperation::Operator::Power;
	}

	throw TranslationException(term.location, "unknown binary operation");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::UnaryOperation::Operator translate(Clingo::AST::UnaryOperator unaryOperator, const Clingo::AST::Term &term)
{
	switch (unaryOperator)
	{
		case Clingo::AST::UnaryOperator::Absolute:
			return ast::UnaryOperation::Operator::Absolute;
		case Clingo::AST::UnaryOperator::Minus:
			throw TranslationException(term.location, "binary operation “minus” currently unsupported");
		case Clingo::AST::UnaryOperator::Negation:
			throw TranslationException(term.location, "binary operation “negation” currently unsupported");
	}

	throw TranslationException(term.location, "unknown unary operation");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Term translate(const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermTranslateVisitor
{
	std::optional<ast::Term> visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		switch (symbol.type())
		{
			case Clingo::SymbolType::Number:
				return ast::Term::make<ast::Integer>(symbol.number());
			case Clingo::SymbolType::Infimum:
				return ast::Term::make<ast::SpecialInteger>(ast::SpecialInteger::Type::Infimum);
			case Clingo::SymbolType::Supremum:
				return ast::Term::make<ast::SpecialInteger>(ast::SpecialInteger::Type::Supremum);
			case Clingo::SymbolType::String:
				return ast::Term::make<ast::String>(std::string(symbol.string()));
			case Clingo::SymbolType::Function:
			{
				auto functionDeclaration = context.findOrCreateFunctionDeclaration(symbol.name(), symbol.arguments().size());

				auto function = ast::Function(functionDeclaration);
				function.arguments.reserve(symbol.arguments().size());

				for (const auto &argument : symbol.arguments())
				{
					auto translatedArgument = visit(argument, term, ruleContext, context, variableStack);

					if (!translatedArgument)
						throw TranslationException(term.location, "could not translate argument");

					function.arguments.emplace_back(std::move(translatedArgument.value()));
				}

				return std::move(function);
			}
		}

		return std::nullopt;
	}

	std::optional<ast::Term> visit(const Clingo::AST::Variable &variable, const Clingo::AST::Term &, RuleContext &ruleContext, Context &, const ast::VariableStack &variableStack)
	{
		const auto matchingVariableDeclaration = variableStack.findUserVariableDeclaration(variable.name);
		const auto isAnonymousVariable = (strcmp(variable.name, "_") == 0);
		const auto isUndeclaredUserVariable = !matchingVariableDeclaration;
		const auto isUndeclared = isAnonymousVariable || isUndeclaredUserVariable;

		if (!isUndeclared)
			return ast::Term::make<ast::Variable>(*matchingVariableDeclaration);

		auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::UserDefined, std::string(variable.name));
		ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));

		// TODO: ast::Term::make is unnecessary and can be removed
		return ast::Term::make<ast::Variable>(ruleContext.freeVariables.back().get());
	}

	std::optional<ast::Term> visit(const Clingo::AST::BinaryOperation &binaryOperation, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translate(binaryOperation.binary_operator, term);
		auto left = translate(binaryOperation.left, ruleContext, context, variableStack);
		auto right = translate(binaryOperation.right, ruleContext, context, variableStack);

		return ast::Term::make<ast::BinaryOperation>(operator_, std::move(left), std::move(right));
	}

	std::optional<ast::Term> visit(const Clingo::AST::UnaryOperation &unaryOperation, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translate(unaryOperation.unary_operator, term);
		auto argument = translate(unaryOperation.argument, ruleContext, context, variableStack);

		return ast::Term::make<ast::UnaryOperation>(operator_, std::move(argument));
	}

	std::optional<ast::Term> visit(const Clingo::AST::Interval &interval, const Clingo::AST::Term &, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		auto left = translate(interval.left, ruleContext, context, variableStack);
		auto right = translate(interval.right, ruleContext, context, variableStack);

		return ast::Term::make<ast::Interval>(std::move(left), std::move(right));
	}

	std::optional<ast::Term> visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		if (function.external)
			throw TranslationException(term.location, "external functions currently unsupported");

		std::vector<ast::Term> arguments;
		arguments.reserve(function.arguments.size());

		for (const auto &argument : function.arguments)
			arguments.emplace_back(translate(argument, ruleContext, context, variableStack));

		auto functionDeclaration = context.findOrCreateFunctionDeclaration(function.name, function.arguments.size());

		return ast::Term::make<ast::Function>(functionDeclaration, std::move(arguments));
	}

	std::optional<ast::Term> visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, RuleContext &, Context &, const ast::VariableStack &)
	{
		throw TranslationException(term.location, "“pool” terms currently unsupported");
		return std::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Term translate(const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
{
	auto translatedTerm = term.data.accept(TermTranslateVisitor(), term, ruleContext, context, variableStack);

	if (!translatedTerm)
		throw TranslationException(term.location, "could not translate term");

	return std::move(translatedTerm.value());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
