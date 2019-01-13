#ifndef __ANTHEM__CHOOSE_VALUE_IN_TERM_H
#define __ANTHEM__CHOOSE_VALUE_IN_TERM_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace direct
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ChooseValueInTerm
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula chooseValueInTerm(const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack);

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula chooseValueInPrimitive(ast::Term &&term, ast::VariableDeclaration &variableDeclaration)
{
	ast::Variable variable(&variableDeclaration);

	return ast::Comparison(ast::Comparison::Operator::Equal, std::move(variable), std::move(term));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ChooseValueInTermVisitor
{
	ast::Formula visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		switch (symbol.type())
		{
			case Clingo::SymbolType::Number:
				return chooseValueInPrimitive(ast::Integer(symbol.number()), variableDeclaration);
			case Clingo::SymbolType::Infimum:
				return chooseValueInPrimitive(ast::SpecialInteger(ast::SpecialInteger::Type::Infimum), variableDeclaration);
			case Clingo::SymbolType::Supremum:
				return chooseValueInPrimitive(ast::SpecialInteger(ast::SpecialInteger::Type::Supremum), variableDeclaration);
			case Clingo::SymbolType::String:
				return chooseValueInPrimitive(ast::String(std::string(symbol.string())), variableDeclaration);
			case Clingo::SymbolType::Function:
			{
				auto functionDeclaration = context.findOrCreateFunctionDeclaration(symbol.name(), symbol.arguments().size());

				ast::Function function(functionDeclaration);
				function.arguments.reserve(symbol.arguments().size());

				ast::Comparison equals(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), std::move(function));

				if (function.arguments.empty())
					return equals;

				ast::VariableDeclarationPointers parameters;
				parameters.reserve(symbol.arguments().size());

				for (int i = 0; i < static_cast<int>(symbol.arguments().size()); i++)
				{
					auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body);
					function.arguments.emplace_back(ast::Variable(variableDeclaration.get()));

					parameters.emplace_back(std::move(variableDeclaration));
				}

				ast::And and_;
				and_.arguments.reserve(parameters.size() + 1);
				and_.arguments.emplace_back(std::move(equals));

				for (int i = 0; i < static_cast<int>(symbol.arguments().size()); i++)
				{
					auto &parameter = *parameters[i];
					const auto &argument = symbol.arguments()[i];

					// TODO: the term argument is incorrect
					auto chooseValueInArgument = visit(argument, term, parameter, context, ruleContext, variableStack);
					and_.arguments.emplace_back(std::move(chooseValueInArgument));
				}

				return ast::Exists(std::move(parameters), std::move(and_));
			}
		}

		throw LogicException("unreachable code, please report to bug tracker");
	}

	ast::Formula visit(const Clingo::AST::Variable &variable, const Clingo::AST::Term &, ast::VariableDeclaration &variableDeclaration, Context &, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		const auto matchingVariableDeclaration = variableStack.findUserVariableDeclaration(variable.name);
		const auto isAnonymousVariable = (strcmp(variable.name, "_") == 0);
		const auto isUndeclaredUserVariable = !matchingVariableDeclaration;
		const auto isUndeclared = isAnonymousVariable || isUndeclaredUserVariable;

		if (!isUndeclared)
			return chooseValueInPrimitive(ast::Variable(*matchingVariableDeclaration), variableDeclaration);

		auto otherVariableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::UserDefined, std::string(variable.name));
		ast::Variable otherVariable(otherVariableDeclaration.get());
		ruleContext.freeVariables.emplace_back(std::move(otherVariableDeclaration));

		return chooseValueInPrimitive(std::move(otherVariable), variableDeclaration);
	}

	ast::Formula visit(const Clingo::AST::BinaryOperation &binaryOperation, const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translate(binaryOperation.binary_operator, term);

		const auto handlePlusMinusAndMultiplication =
			[&]()
			{
				ast::VariableDeclarationPointers parameters;
				parameters.reserve(2);

				for (int i = 0; i < 2; i++)
					parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

				ast::BinaryOperation translatedBinaryOperation(operator_, ast::Variable(parameters[0].get()), ast::Variable(parameters[1].get()));

				std::vector<ast::Formula> andArguments;
				andArguments.reserve(3);

				ast::Comparison equals(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), std::move(translatedBinaryOperation));
				andArguments.emplace_back(std::move(equals));

				auto chooseValueFromLeftArgument = chooseValueInTerm(binaryOperation.left, *parameters[0], context, ruleContext, variableStack);
				andArguments.emplace_back(std::move(chooseValueFromLeftArgument));

				auto chooseValueFromRightArgument = chooseValueInTerm(binaryOperation.right, *parameters[1], context, ruleContext, variableStack);
				andArguments.emplace_back(std::move(chooseValueFromRightArgument));

				ast::And and_(std::move(andArguments));

				return ast::Exists(std::move(parameters), std::move(and_));
			};

		const auto handleDivisonAndModulo =
			[&]()
			{
				ast::VariableDeclarationPointers parameters;
				parameters.reserve(4);

				for (int i = 0; i < 4; i++)
					parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

				auto &parameterI = parameters[0];
				auto &parameterJ = parameters[1];
				auto &parameterQ = parameters[2];
				auto &parameterR = parameters[3];

				ast::And and_;
				and_.arguments.reserve(7);

				ast::BinaryOperation jTimesQ(ast::BinaryOperation::Operator::Multiplication, ast::Variable(parameterJ.get()), ast::Variable(parameterQ.get()));
				ast::BinaryOperation jTimesQPlusR(ast::BinaryOperation::Operator::Plus, std::move(jTimesQ), ast::Variable(parameterR.get()));
				ast::Comparison iEqualsJTimesQPlusR(ast::Comparison::Operator::Equal, ast::Variable(parameterI.get()), std::move(jTimesQPlusR));
				and_.arguments.emplace_back(std::move(iEqualsJTimesQPlusR));

				auto chooseIInT1 = chooseValueInTerm(binaryOperation.left, *parameterI, context, ruleContext, variableStack);
				and_.arguments.emplace_back(std::move(chooseIInT1));

				auto chooseJInT2 = chooseValueInTerm(binaryOperation.right, *parameterJ, context, ruleContext, variableStack);
				and_.arguments.emplace_back(std::move(chooseJInT2));

				ast::Comparison jNotEqualTo0(ast::Comparison::Operator::NotEqual, ast::Variable(parameterJ.get()), ast::Integer(0));
				and_.arguments.emplace_back(std::move(jNotEqualTo0));

				ast::Comparison rGreaterOrEqualTo0(ast::Comparison::Operator::GreaterEqual, ast::Variable(parameterR.get()), ast::Integer(0));
				and_.arguments.emplace_back(std::move(rGreaterOrEqualTo0));

				ast::Comparison rLessThanQ(ast::Comparison::Operator::LessThan, ast::Variable(parameterR.get()), ast::Variable(parameterQ.get()));
				and_.arguments.emplace_back(std::move(rLessThanQ));

				switch (operator_)
				{
					case ast::BinaryOperation::Operator::Division:
					{
						ast::Comparison zEqualToQ(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), ast::Variable(parameterQ.get()));
						and_.arguments.emplace_back(std::move(zEqualToQ));
					}
					case ast::BinaryOperation::Operator::Modulo:
					{
						ast::Comparison zEqualToR(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), ast::Variable(parameterR.get()));
						and_.arguments.emplace_back(std::move(zEqualToR));
					}
					default:
						throw TranslationException("unexpected binary operation, please report this to the bug tracker");
				}

				return ast::Exists(std::move(parameters), std::move(and_));
			};

		switch (operator_)
		{
			case ast::BinaryOperation::Operator::Plus:
				return handlePlusMinusAndMultiplication();
			case ast::BinaryOperation::Operator::Minus:
				return handlePlusMinusAndMultiplication();
			case ast::BinaryOperation::Operator::Multiplication:
				return handlePlusMinusAndMultiplication();
			case ast::BinaryOperation::Operator::Division:
				return handleDivisonAndModulo();
			case ast::BinaryOperation::Operator::Modulo:
				return handleDivisonAndModulo();
			case ast::BinaryOperation::Operator::Power:
				throw TranslationException("binary operator “power” not yet supported in this context");
		}

		throw LogicException("unreachable code, please report to bug tracker");
	}

	ast::Formula visit(const Clingo::AST::UnaryOperation &, const Clingo::AST::Term &, ast::VariableDeclaration &, Context &, RuleContext &, const ast::VariableStack &)
	{
		throw TranslationException("unary operations not yet supported in this context");
	}

	ast::Formula visit(const Clingo::AST::Interval &interval, const Clingo::AST::Term &, ast::VariableDeclaration &variableDeclaration, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
	{
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(3);

		for (int i = 0; i < 3; i++)
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

		auto &parameterI = parameters[0];
		auto &parameterJ = parameters[1];
		auto &parameterK = parameters[2];

		ast::And and_;
		and_.arguments.reserve(5);

		auto chooseIInT1 = chooseValueInTerm(interval.left, *parameterI, context, ruleContext, variableStack);
		and_.arguments.emplace_back(std::move(chooseIInT1));

		auto chooseJInT2 = chooseValueInTerm(interval.right, *parameterJ, context, ruleContext, variableStack);
		and_.arguments.emplace_back(std::move(chooseJInT2));

		ast::Comparison iLessOrEqualToK(ast::Comparison::Operator::LessEqual, ast::Variable(parameterI.get()), ast::Variable(parameterK.get()));
		and_.arguments.emplace_back(std::move(iLessOrEqualToK));

		ast::Comparison kLessOrEqualToJ(ast::Comparison::Operator::LessEqual, ast::Variable(parameterK.get()), ast::Variable(parameterJ.get()));
		and_.arguments.emplace_back(std::move(kLessOrEqualToJ));

		ast::Comparison zEqualToK(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), ast::Variable(parameterK.get()));
		and_.arguments.emplace_back(std::move(zEqualToK));

		return ast::Exists(std::move(parameters), std::move(and_));
	}

	ast::Formula visit(const Clingo::AST::Function &, const Clingo::AST::Term &term, ast::VariableDeclaration &, Context &, RuleContext &, const ast::VariableStack &)
	{
		throw TranslationException(term.location, "functions not yet supported in this context");
	}

	ast::Formula visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, ast::VariableDeclaration &, Context &, RuleContext &, const ast::VariableStack &)
	{
		throw TranslationException(term.location, "“pool” terms not yet unsupported in this context");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula chooseValueInTerm(const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, RuleContext &ruleContext, const ast::VariableStack &variableStack)
{
	return term.data.accept(ChooseValueInTermVisitor(), term, variableDeclaration, context, ruleContext, variableStack);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
