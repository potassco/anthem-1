#include <anthem/IntegerVariableDetection.h>

#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Exception.h>
#include <anthem/Simplification.h>
#include <anthem/output/AST.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IntegerVariableDetection
//
////////////////////////////////////////////////////////////////////////////////////////////////////

using VariableDomainMap = std::map<const ast::VariableDeclaration *, ast::Domain>;

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Domain domain(const ast::Variable &variable, VariableDomainMap &variableDomainMap)
{
	if (variable.declaration->domain != ast::Domain::Unknown)
		return variable.declaration->domain;

	const auto match = variableDomainMap.find(variable.declaration);

	if (match == variableDomainMap.end())
		return ast::Domain::Unknown;

	return match->second;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void clearVariableDomainMap(VariableDomainMap &variableDomainMap)
{
	for (auto &variableDeclaration : variableDomainMap)
		variableDeclaration.second = ast::Domain::Unknown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class OperationResult
{
	Unchanged,
	Changed,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class EvaluationResult
{
	True,
	False,
	Unknown,
	Error,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

EvaluationResult isArithmetic(const ast::Term &term, VariableDomainMap &variableDomainMap);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct IsTermArithmeticVisitor
{
	static EvaluationResult visit(const ast::BinaryOperation &binaryOperation, VariableDomainMap &variableDomainMap)
	{
		const auto isLeftArithemtic = isArithmetic(binaryOperation.left, variableDomainMap);
		const auto isRightArithmetic = isArithmetic(binaryOperation.right, variableDomainMap);

		if (isLeftArithemtic == EvaluationResult::Error || isRightArithmetic == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (isLeftArithemtic == EvaluationResult::False || isRightArithmetic == EvaluationResult::False)
			return EvaluationResult::Error;

		if (isLeftArithemtic == EvaluationResult::Unknown || isRightArithmetic == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::True;
	}

	static EvaluationResult visit(const ast::Boolean &, VariableDomainMap &)
	{
		return EvaluationResult::False;
	}

	static EvaluationResult visit(const ast::Function &function, VariableDomainMap &)
	{
		switch (function.declaration->domain)
		{
			case ast::Domain::General:
				return EvaluationResult::False;
			case ast::Domain::Integer:
				return EvaluationResult::True;
			case ast::Domain::Unknown:
				return EvaluationResult::Unknown;
		}

		return EvaluationResult::Unknown;
	}

	static EvaluationResult visit(const ast::Integer &, VariableDomainMap &)
	{
		return EvaluationResult::True;
	}

	static EvaluationResult visit(const ast::Interval &interval, VariableDomainMap &variableDomainMap)
	{
		const auto isFromArithmetic = isArithmetic(interval.from, variableDomainMap);
		const auto isToArithmetic = isArithmetic(interval.to, variableDomainMap);

		if (isFromArithmetic == EvaluationResult::Error || isToArithmetic == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (isFromArithmetic == EvaluationResult::False || isToArithmetic == EvaluationResult::False)
			return EvaluationResult::Error;

		if (isFromArithmetic == EvaluationResult::Unknown || isToArithmetic == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::True;
	}

	static EvaluationResult visit(const ast::SpecialInteger &, VariableDomainMap &)
	{
		return EvaluationResult::False;
	}

	static EvaluationResult visit(const ast::String &, VariableDomainMap &)
	{
		return EvaluationResult::False;
	}

	static EvaluationResult visit(const ast::UnaryOperation &unaryOperation, VariableDomainMap &variableDomainMap)
	{
		const auto isArgumentArithmetic = isArithmetic(unaryOperation.argument, variableDomainMap);

		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				return (isArgumentArithmetic == EvaluationResult::False ? EvaluationResult::Error : isArgumentArithmetic);
		}

		return EvaluationResult::Unknown;
	}

	static EvaluationResult visit(const ast::Variable &variable, VariableDomainMap &variableDomainMap)
	{
		switch (domain(variable, variableDomainMap))
		{
			case ast::Domain::General:
				return EvaluationResult::False;
			case ast::Domain::Integer:
				return EvaluationResult::True;
			case ast::Domain::Unknown:
				return EvaluationResult::Unknown;
		}

		return EvaluationResult::Unknown;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

EvaluationResult isArithmetic(const ast::Term &term, VariableDomainMap &variableDomainMap)
{
	return term.accept(IsTermArithmeticVisitor(), variableDomainMap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

EvaluationResult evaluate(const ast::Formula &formula, VariableDomainMap &variableDomainMap);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct EvaluateFormulaVisitor
{
	static EvaluationResult visit(const ast::And &and_, VariableDomainMap &variableDomainMap)
	{
		bool someFalse = false;
		bool someUnknown = false;

		for (const auto &argument : and_.arguments)
		{
			const auto result = evaluate(argument, variableDomainMap);

			switch (result)
			{
				case EvaluationResult::Error:
					return EvaluationResult::Error;
				case EvaluationResult::True:
					break;
				case EvaluationResult::False:
					someFalse = true;
					break;
				case EvaluationResult::Unknown:
					someUnknown = true;
					break;
			}
		}

		if (someFalse)
			return EvaluationResult::False;

		if (someUnknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::True;
	}

	static EvaluationResult visit(const ast::Biconditional &biconditional, VariableDomainMap &variableDomainMap)
	{
		const auto leftResult = evaluate(biconditional.left, variableDomainMap);
		const auto rightResult = evaluate(biconditional.right, variableDomainMap);

		if (leftResult == EvaluationResult::Error || rightResult == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (leftResult == EvaluationResult::Unknown || rightResult == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		return (leftResult == rightResult ? EvaluationResult::True : EvaluationResult::False);
	}

	static EvaluationResult visit(const ast::Boolean &boolean, VariableDomainMap &)
	{
		return (boolean.value == true ? EvaluationResult::True : EvaluationResult::False);
	}

	static EvaluationResult visit(const ast::Comparison &comparison, VariableDomainMap &variableDomainMap)
	{
		const auto isLeftArithmetic = isArithmetic(comparison.left, variableDomainMap);
		const auto isRightArithmetic = isArithmetic(comparison.right, variableDomainMap);

		if (isLeftArithmetic == EvaluationResult::Error || isRightArithmetic == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (isLeftArithmetic == EvaluationResult::Unknown || isRightArithmetic == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		if (isLeftArithmetic == isRightArithmetic)
			return EvaluationResult::Unknown;

		// Handle the case where one side is arithmetic but the other one isn’t
		switch (comparison.operator_)
		{
			case ast::Comparison::Operator::Equal:
				return EvaluationResult::False;
			case ast::Comparison::Operator::NotEqual:
				return EvaluationResult::True;
			default:
				// TODO: implement more cases
				return EvaluationResult::Unknown;
		}
	}

	static EvaluationResult visit(const ast::Exists &exists, VariableDomainMap &variableDomainMap)
	{
		return evaluate(exists.argument, variableDomainMap);
	}

	static EvaluationResult visit(const ast::ForAll &forAll, VariableDomainMap &variableDomainMap)
	{
		return evaluate(forAll.argument, variableDomainMap);
	}

	static EvaluationResult visit(const ast::Implies &implies, VariableDomainMap &variableDomainMap)
	{
		const auto antecedentResult = evaluate(implies.antecedent, variableDomainMap);
		const auto consequentResult = evaluate(implies.consequent, variableDomainMap);

		if (antecedentResult == EvaluationResult::Error || consequentResult == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (antecedentResult == EvaluationResult::False)
			return EvaluationResult::True;

		if (consequentResult == EvaluationResult::True)
			return EvaluationResult::True;

		if (antecedentResult == EvaluationResult::True && consequentResult == EvaluationResult::False)
			return EvaluationResult::False;

		return EvaluationResult::Unknown;
	}

	static EvaluationResult visit(const ast::In &in, VariableDomainMap &variableDomainMap)
	{
		const auto isElementArithmetic = isArithmetic(in.element, variableDomainMap);
		const auto isSetArithmetic = isArithmetic(in.set, variableDomainMap);

		if (isElementArithmetic == EvaluationResult::Error || isSetArithmetic == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (isElementArithmetic == EvaluationResult::Unknown || isSetArithmetic == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		if (isElementArithmetic == isSetArithmetic)
			return EvaluationResult::Unknown;

		// If one side is arithmetic, but the other one isn’t, set inclusion is never satisfied
		return EvaluationResult::False;
	}

	static EvaluationResult visit(const ast::Not &not_, VariableDomainMap &variableDomainMap)
	{
		const auto result = evaluate(not_.argument, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::Unknown)
			return result;

		return (result == EvaluationResult::True ? EvaluationResult::False : EvaluationResult::True);
	}

	static EvaluationResult visit(const ast::Or &or_, VariableDomainMap &variableDomainMap)
	{
		bool someTrue = false;
		bool someUnknown = false;

		for (const auto &argument : or_.arguments)
		{
			const auto result = evaluate(argument, variableDomainMap);

			switch (result)
			{
				case EvaluationResult::Error:
					return EvaluationResult::Error;
				case EvaluationResult::True:
					someTrue = true;
					break;
				case EvaluationResult::False:
					break;
				case EvaluationResult::Unknown:
					someUnknown = true;
					break;
			}
		}

		if (someTrue)
			return EvaluationResult::True;

		if (someUnknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::False;
	}

	static EvaluationResult visit(const ast::Predicate &predicate, VariableDomainMap &variableDomainMap)
	{
		assert(predicate.arguments.size() == predicate.declaration->arity());

		for (size_t i = 0; i < predicate.arguments.size(); i++)
		{
			const auto &argument = predicate.arguments[i];
			const auto &parameter = predicate.declaration->parameters[i];

			if (parameter.domain != ast::Domain::Integer)
				continue;

			const auto isArgumentArithmetic = isArithmetic(argument, variableDomainMap);

			if (isArgumentArithmetic == EvaluationResult::Error || isArgumentArithmetic == EvaluationResult::False)
				return isArgumentArithmetic;
		}

		return EvaluationResult::Unknown;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

EvaluationResult evaluate(const ast::Formula &formula, VariableDomainMap &variableDomainMap)
{
	return formula.accept(EvaluateFormulaVisitor(), variableDomainMap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

OperationResult detectIntegerVariables(ast::Formula &formula, ast::Formula &definition, VariableDomainMap &variableDomainMap);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct DetectIntegerVariablesVisitor
{
	static OperationResult visit(ast::And &and_, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		auto operationResult = OperationResult::Unchanged;

		for (auto &argument : and_.arguments)
			if (detectIntegerVariables(argument, definition, variableDomainMap) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::Biconditional &biconditional, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		auto operationResult = OperationResult::Unchanged;

		if (detectIntegerVariables(biconditional.left, definition, variableDomainMap) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		if (detectIntegerVariables(biconditional.right, definition, variableDomainMap) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::Boolean &, ast::Formula &, VariableDomainMap &)
	{
		return OperationResult::Unchanged;
	}

	static OperationResult visit(ast::Comparison &, ast::Formula &, VariableDomainMap &)
	{
		return OperationResult::Unchanged;
	}

	static OperationResult visit(ast::Exists &exists, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		auto operationResult = OperationResult::Unchanged;

		if (detectIntegerVariables(exists.argument, definition, variableDomainMap) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		for (auto &variableDeclaration : exists.variables)
		{
			if (variableDeclaration->domain != ast::Domain::Unknown)
				continue;

			clearVariableDomainMap(variableDomainMap);

			auto argumentResult = evaluate(exists.argument, variableDomainMap);
			auto definitionResult = evaluate(definition, variableDomainMap);

			if (argumentResult == EvaluationResult::Error || argumentResult == EvaluationResult::False
			    || definitionResult == EvaluationResult::Error || definitionResult == EvaluationResult::False)
			{
				continue;
			}

			// As a hypothesis, make the parameter’s domain noninteger
			variableDomainMap[variableDeclaration.get()] = ast::Domain::General;

			argumentResult = evaluate(exists.argument, variableDomainMap);
			definitionResult = evaluate(definition, variableDomainMap);

			if (argumentResult == EvaluationResult::Error || argumentResult == EvaluationResult::False
			    || definitionResult == EvaluationResult::Error || definitionResult == EvaluationResult::False)
			{
				// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
				operationResult = OperationResult::Changed;
				variableDeclaration->domain = ast::Domain::Integer;
			}
		}

		return operationResult;
	}

	static OperationResult visit(ast::ForAll &forAll, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		auto operationResult = OperationResult::Unchanged;

		if (detectIntegerVariables(forAll.argument, definition, variableDomainMap) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		for (auto &variableDeclaration : forAll.variables)
		{
			if (variableDeclaration->domain != ast::Domain::Unknown)
				continue;

			clearVariableDomainMap(variableDomainMap);

			auto argumentResult = evaluate(forAll.argument, variableDomainMap);
			auto definitionResult = evaluate(definition, variableDomainMap);

			if (argumentResult == EvaluationResult::Error || argumentResult == EvaluationResult::False
			    || definitionResult == EvaluationResult::Error || definitionResult == EvaluationResult::False)
			{
				continue;
			}

			// As a hypothesis, make the parameter’s domain noninteger
			variableDomainMap[variableDeclaration.get()] = ast::Domain::General;

			argumentResult = evaluate(forAll.argument, variableDomainMap);
			definitionResult = evaluate(definition, variableDomainMap);

			if (argumentResult == EvaluationResult::Error || argumentResult == EvaluationResult::False
			    || definitionResult == EvaluationResult::Error || definitionResult == EvaluationResult::False)
			{
				// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
				operationResult = OperationResult::Changed;
				variableDeclaration->domain = ast::Domain::Integer;
			}
		}

		return operationResult;
	}

	static OperationResult visit(ast::Implies &implies, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		auto operationResult = OperationResult::Unchanged;

		if (detectIntegerVariables(implies.antecedent, definition, variableDomainMap) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		if (detectIntegerVariables(implies.consequent, definition, variableDomainMap) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::In &, ast::Formula &, VariableDomainMap &)
	{
		return OperationResult::Unchanged;
	}

	static OperationResult visit(ast::Not &not_, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		return detectIntegerVariables(not_.argument, definition, variableDomainMap);
	}

	static OperationResult visit(ast::Or &or_, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		auto operationResult = OperationResult::Unchanged;

		for (auto &argument : or_.arguments)
			if (detectIntegerVariables(argument, definition, variableDomainMap) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::Predicate &predicate, ast::Formula &, VariableDomainMap &)
	{
		auto operationResult = OperationResult::Unchanged;

		assert(predicate.arguments.size() == predicate.declaration->arity());

		// Propagate integer domains from predicates to variables
		for (size_t i = 0; i < predicate.arguments.size(); i++)
		{
			auto &variableArgument = predicate.arguments[i];
			auto &parameter = predicate.declaration->parameters[i];

			if (parameter.domain != ast::Domain::Integer)
				continue;

			if (!variableArgument.is<ast::Variable>())
				continue;

			auto &variable = variableArgument.get<ast::Variable>();

			if (variable.declaration->domain == ast::Domain::Integer)
				continue;

			operationResult = OperationResult::Changed;
			variable.declaration->domain = ast::Domain::Integer;
		}

		return operationResult;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

OperationResult detectIntegerVariables(ast::Formula &formula, ast::Formula &definition, VariableDomainMap &variableDomainMap)
{
	return formula.accept(DetectIntegerVariablesVisitor(), definition, variableDomainMap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Assumes the completed formulas to be in translated but not simplified form.
// That is, completed formulas are either variable-free or universally quantified
void detectIntegerVariables(std::vector<ast::Formula> &completedFormulas)
{
	VariableDomainMap variableDomainMap;
	auto operationResult = OperationResult::Changed;

	while (operationResult == OperationResult::Changed)
	{
		operationResult = OperationResult::Unchanged;

		for (auto &completedFormula : completedFormulas)
		{
			if (detectIntegerVariables(completedFormula, completedFormula, variableDomainMap) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

			if (!completedFormula.is<ast::ForAll>())
				continue;

			auto &forAll = completedFormula.get<ast::ForAll>();

			if (!forAll.argument.is<ast::Biconditional>())
				continue;

			auto &biconditional = forAll.argument.get<ast::Biconditional>();

			if (!biconditional.left.is<ast::Predicate>())
				continue;

			auto &predicate = biconditional.left.get<ast::Predicate>();
			auto &definition = biconditional.right;

			assert(predicate.arguments.size() == predicate.declaration->arity());

			if (detectIntegerVariables(definition, definition, variableDomainMap) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

			for (size_t i = 0; i < predicate.arguments.size(); i++)
			{
				auto &variableArgument = predicate.arguments[i];
				auto &parameter = predicate.declaration->parameters[i];

				assert(variableArgument.is<ast::Variable>());

				auto &variable = variableArgument.get<ast::Variable>();

				if (parameter.domain != ast::Domain::Unknown)
					continue;

				clearVariableDomainMap(variableDomainMap);

				auto result = evaluate(definition, variableDomainMap);

				if (result == EvaluationResult::Error || result == EvaluationResult::False)
					continue;

				// As a hypothesis, make the parameter’s domain noninteger
				variableDomainMap[variable.declaration] = ast::Domain::General;

				result = evaluate(definition, variableDomainMap);

				if (result == EvaluationResult::Error || result == EvaluationResult::False)
				{
					// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
					operationResult = OperationResult::Changed;
					variable.declaration->domain = ast::Domain::Integer;
					parameter.domain = ast::Domain::Integer;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
