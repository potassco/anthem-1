#ifndef __ANTHEM__TERM_H
#define __ANTHEM__TERM_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
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

ast::BinaryOperation::Operator translate(Clingo::AST::BinaryOperator binaryOperator, const Clingo::AST::Term &term, Context &context)
{
	switch (binaryOperator)
	{
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
		default:
			throwErrorAtLocation(term.location, "“binary operation” terms currently unsupported", context);
	}

	return ast::BinaryOperation::Operator::Plus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Term translate(const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermTranslateVisitor
{
	std::experimental::optional<ast::Term> visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
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
				auto function = ast::Term::make<ast::Function>(symbol.name());
				// TODO: remove workaround
				auto &functionRaw = function.get<ast::Function>();
				functionRaw.arguments.reserve(symbol.arguments().size());

				for (const auto &argument : symbol.arguments())
				{
					auto translatedArgument = visit(argument, term, context, ruleContext, variableStack);

					if (!translatedArgument)
						throwErrorAtLocation(term.location, "could not translate argument", context);

					functionRaw.arguments.emplace_back(std::move(translatedArgument.value()));
				}

				return std::move(function);
			}
		}

		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Term> visit(const Clingo::AST::Variable &variable, const Clingo::AST::Term &, Context &, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		const auto matchingVariableDeclaration = variableStack.findUserVariableDeclaration(variable.name);
		const auto isAnonymousVariable = (strcmp(variable.name, "_") == 0);
		const auto isUndeclaredUserVariable = !matchingVariableDeclaration;
		const auto isUndeclared = isAnonymousVariable || isUndeclaredUserVariable;

		if (!isUndeclared)
			return ast::Term::make<ast::Variable>(*matchingVariableDeclaration);

		auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::UserDefined, std::string(variable.name));
		ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));

		return ast::Term::make<ast::Variable>(ruleContext.freeVariables.back().get());
	}

	std::experimental::optional<ast::Term> visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &term, Context &context, RuleContext &, const ast::VariableStack &)
	{
		throwErrorAtLocation(term.location, "“unary operation” terms currently unsupported", context);
		return std::experimental::nullopt;
	}

	std::experimental::optional<ast::Term> visit(const Clingo::AST::BinaryOperation &binaryOperation, const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translate(binaryOperation.binary_operator, term, context);
		auto left = translate(binaryOperation.left, context, ruleContext, variableStack);
		auto right = translate(binaryOperation.right, context, ruleContext, variableStack);

		return ast::Term::make<ast::BinaryOperation>(operator_, std::move(left), std::move(right));
	}

	std::experimental::optional<ast::Term> visit(const Clingo::AST::Interval &interval, const Clingo::AST::Term &, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		auto left = translate(interval.left, context, ruleContext, variableStack);
		auto right = translate(interval.right, context, ruleContext, variableStack);

		return ast::Term::make<ast::Interval>(std::move(left), std::move(right));
	}

	std::experimental::optional<ast::Term> visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		if (function.external)
			throwErrorAtLocation(term.location, "external functions currently unsupported", context);

		std::vector<ast::Term> arguments;
		arguments.reserve(function.arguments.size());

		for (const auto &argument : function.arguments)
			arguments.emplace_back(translate(argument, context, ruleContext, variableStack));

		return ast::Term::make<ast::Function>(function.name, std::move(arguments));
	}

	std::experimental::optional<ast::Term> visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, Context &context, RuleContext &, const ast::VariableStack &)
	{
		throwErrorAtLocation(term.location, "“pool” terms currently unsupported", context);
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Term translate(const Clingo::AST::Term &term, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
{
	auto translatedTerm = term.data.accept(TermTranslateVisitor(), term, context, ruleContext, variableStack);

	if (!translatedTerm)
		throwErrorAtLocation(term.location, "could not translate term", context);

	return std::move(translatedTerm.value());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
