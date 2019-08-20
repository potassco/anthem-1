#ifndef __ANTHEM__TRANSLATION_COMMON__CHOOSE_VALUE_IN_TERM_H
#define __ANTHEM__TRANSLATION_COMMON__CHOOSE_VALUE_IN_TERM_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Exception.h>
#include <anthem/Utils.h>
#include <anthem/translation-common/Term.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ChooseValueInTerm
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula chooseValueInTerm(const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, ast::VariableDeclarationPointers &freeVariables, const ast::VariableStack &variableStack);

////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::Formula chooseValueInPrimitive(ast::Term &&term, ast::VariableDeclaration &variableDeclaration)
{
	ast::Variable variable(&variableDeclaration);

	return ast::Comparison(ast::Comparison::Operator::Equal, std::move(variable), std::move(term));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ChooseValueInTermVisitor
{
	ast::Formula visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, ast::VariableDeclarationPointers &, const ast::VariableStack &)
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
				// Functions with arguments are represented as Clingo::AST::Function by the parser. At this
				// point, we only have to handle (0-ary) constants
				if (!symbol.arguments().empty())
					throw LogicException(term.location, "unexpected arguments, expected (0-ary) constant symbol, please report to bug tracker");

				auto constantDeclaration = context.findOrCreateFunctionDeclaration(symbol.name(), 0);
				return chooseValueInPrimitive(ast::Function(constantDeclaration), variableDeclaration);
			}
			default:
				throw LogicException("unexpected symbol type, please report to bug tracker");
		}
	}

	ast::Formula visit(const Clingo::AST::Variable &variable, const Clingo::AST::Term &, ast::VariableDeclaration &variableDeclaration, Context &, ast::VariableDeclarationPointers &freeVariables, const ast::VariableStack &variableStack)
	{
		const auto matchingVariableDeclaration = variableStack.findUserVariableDeclaration(variable.name);
		const auto isAnonymousVariable = (strcmp(variable.name, "_") == 0);
		const auto isUndeclaredUserVariable = !matchingVariableDeclaration;
		const auto isUndeclared = isAnonymousVariable || isUndeclaredUserVariable;

		if (!isUndeclared)
			return chooseValueInPrimitive(ast::Variable(*matchingVariableDeclaration), variableDeclaration);

		auto otherVariableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::UserDefined, std::string(variable.name));
		otherVariableDeclaration->domain = Domain::Program;
		ast::Variable otherVariable(otherVariableDeclaration.get());
		freeVariables.emplace_back(std::move(otherVariableDeclaration));

		return chooseValueInPrimitive(std::move(otherVariable), variableDeclaration);
	}

	ast::Formula visit(const Clingo::AST::BinaryOperation &binaryOperation, const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, ast::VariableDeclarationPointers &freeVariables, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translationCommon::translate(binaryOperation.binary_operator, term);

		const auto handlePlusMinusAndMultiplication =
			[&]()
			{
				ast::VariableDeclarationPointers parameters;
				parameters.reserve(2);

				for (auto i = 0; i < 2; i++)
				{
					parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
					parameters.back()->domain = Domain::Integer;
				}

				ast::BinaryOperation translatedBinaryOperation(operator_, ast::Variable(parameters[0].get()), ast::Variable(parameters[1].get()));

				std::vector<ast::Formula> andArguments;
				andArguments.reserve(3);

				ast::Comparison equals(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), std::move(translatedBinaryOperation));
				andArguments.emplace_back(std::move(equals));

				auto chooseValueFromLeftArgument = chooseValueInTerm(binaryOperation.left, *parameters[0], context, freeVariables, variableStack);
				andArguments.emplace_back(std::move(chooseValueFromLeftArgument));

				auto chooseValueFromRightArgument = chooseValueInTerm(binaryOperation.right, *parameters[1], context, freeVariables, variableStack);
				andArguments.emplace_back(std::move(chooseValueFromRightArgument));

				ast::And and_(std::move(andArguments));

				return ast::Exists(std::move(parameters), std::move(and_));
			};

		const auto handleDivisonAndModulo =
			[&]()
			{
				ast::VariableDeclarationPointers parameters;
				parameters.reserve(4);

				for (auto i = 0; i < 4; i++)
				{
					parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
					parameters.back()->domain = Domain::Integer;
				}

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

				auto chooseIInT1 = chooseValueInTerm(binaryOperation.left, *parameterI, context, freeVariables, variableStack);
				and_.arguments.emplace_back(std::move(chooseIInT1));

				auto chooseJInT2 = chooseValueInTerm(binaryOperation.right, *parameterJ, context, freeVariables, variableStack);
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
						break;
					}
					case ast::BinaryOperation::Operator::Modulo:
					{
						ast::Comparison zEqualToR(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), ast::Variable(parameterR.get()));
						and_.arguments.emplace_back(std::move(zEqualToR));
						break;
					}
					default:
						throw LogicException("unexpected binary binary operator, please report this to the bug tracker");
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
				throw TranslationException("binary operator “power” not yet supported");
			default:
				throw LogicException("unexpected binary operator, please report to bug tracker");
		}
	}

	ast::Formula visit(const Clingo::AST::UnaryOperation &unaryOperation, const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, ast::VariableDeclarationPointers &freeVariables, const ast::VariableStack &variableStack)
	{
		switch (unaryOperation.unary_operator)
		{
			case Clingo::AST::UnaryOperator::Absolute:
				throw TranslationException(term.location, "unary operation “absolute value” not yet supported");
			case Clingo::AST::UnaryOperator::Minus:
			{
				ast::VariableDeclarationPointers parameters;
				parameters.reserve(1);
				parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
				parameters.back()->domain = Domain::Integer;

				auto &parameterZPrime = parameters[0];

				ast::And and_;
				and_.arguments.reserve(2);

				ast::UnaryOperation minusZPrime = ast::UnaryOperation(ast::UnaryOperation::Operator::Minus, ast::Variable(parameterZPrime.get()));
				ast::Comparison equals(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), std::move(minusZPrime));
				and_.arguments.emplace_back(std::move(equals));

				auto chooseZPrimeInTPrime = chooseValueInTerm(unaryOperation.argument, *parameterZPrime, context, freeVariables, variableStack);
				and_.arguments.emplace_back(std::move(chooseZPrimeInTPrime));

				return ast::Exists(std::move(parameters), std::move(and_));
			}
			case Clingo::AST::UnaryOperator::Negation:
				throw TranslationException(term.location, "unary operation “negation” not yet supported");
			default:
				throw LogicException("unexpected unary operator, please report to bug tracker");
		}
	}

	ast::Formula visit(const Clingo::AST::Interval &interval, const Clingo::AST::Term &, ast::VariableDeclaration &variableDeclaration, Context &context, ast::VariableDeclarationPointers &freeVariables, const ast::VariableStack &variableStack)
	{
		ast::VariableDeclarationPointers parameters;
		parameters.reserve(3);

		for (auto i = 0; i < 3; i++)
		{
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
			parameters.back()->domain = Domain::Integer;
		}

		auto &parameterI = parameters[0];
		auto &parameterJ = parameters[1];
		auto &parameterK = parameters[2];

		ast::And and_;
		and_.arguments.reserve(5);

		auto chooseIInT1 = chooseValueInTerm(interval.left, *parameterI, context, freeVariables, variableStack);
		and_.arguments.emplace_back(std::move(chooseIInT1));

		auto chooseJInT2 = chooseValueInTerm(interval.right, *parameterJ, context, freeVariables, variableStack);
		and_.arguments.emplace_back(std::move(chooseJInT2));

		ast::Comparison iLessOrEqualToK(ast::Comparison::Operator::LessEqual, ast::Variable(parameterI.get()), ast::Variable(parameterK.get()));
		and_.arguments.emplace_back(std::move(iLessOrEqualToK));

		ast::Comparison kLessOrEqualToJ(ast::Comparison::Operator::LessEqual, ast::Variable(parameterK.get()), ast::Variable(parameterJ.get()));
		and_.arguments.emplace_back(std::move(kLessOrEqualToJ));

		ast::Comparison zEqualToK(ast::Comparison::Operator::Equal, ast::Variable(&variableDeclaration), ast::Variable(parameterK.get()));
		and_.arguments.emplace_back(std::move(zEqualToK));

		return ast::Exists(std::move(parameters), std::move(and_));
	}

	ast::Formula visit(const Clingo::AST::Function &, const Clingo::AST::Term &term, ast::VariableDeclaration &, Context &, ast::VariableDeclarationPointers &, const ast::VariableStack &)
	{
		throw TranslationException(term.location, "symbolic functions not yet supported");
	}

	ast::Formula visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, ast::VariableDeclaration &, Context &, ast::VariableDeclarationPointers &, const ast::VariableStack &)
	{
		throw TranslationException(term.location, "pools not yet supported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::Formula chooseValueInTerm(const Clingo::AST::Term &term, ast::VariableDeclaration &variableDeclaration, Context &context, ast::VariableDeclarationPointers &freeVariables, const ast::VariableStack &variableStack)
{
	return term.data.accept(ChooseValueInTermVisitor(), term, variableDeclaration, context, freeVariables, variableStack);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
