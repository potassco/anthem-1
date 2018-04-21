#include <anthem/IntegerVariableDetection.h>

#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Exception.h>
#include <anthem/Simplification.h>
#include <anthem/Utils.h>
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

template <class Functor>
struct ForEachVariableDeclarationVisitor
{
	template <class... Arguments>
	static OperationResult visit(ast::And &and_, Arguments &&... arguments)
	{
		auto operationResult = OperationResult::Unchanged;

		for (auto &argument : and_.arguments)
			if (argument.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Biconditional &biconditional, Arguments &&... arguments)
	{
		auto operationResult = OperationResult::Unchanged;

		if (biconditional.left.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		if (biconditional.right.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		return operationResult;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Boolean &, Arguments &&...)
	{
		return OperationResult::Unchanged;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Comparison &, Arguments &&...)
	{
		return OperationResult::Unchanged;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Exists &exists, Arguments &&... arguments)
	{
		auto operationResult = OperationResult::Unchanged;

		if (exists.argument.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		for (auto &variableDeclaration : exists.variables)
			if (Functor()(*variableDeclaration, exists.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	template <class... Arguments>
	static OperationResult visit(ast::ForAll &forAll, Arguments &&... arguments)
	{
		auto operationResult = OperationResult::Unchanged;

		if (forAll.argument.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		for (auto &variableDeclaration : forAll.variables)
			if (Functor()(*variableDeclaration, forAll.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Implies &implies, Arguments &&... arguments)
	{
		auto operationResult = OperationResult::Unchanged;

		if (implies.antecedent.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		if (implies.consequent.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		return operationResult;
	}

	template <class... Arguments>
	static OperationResult visit(ast::In &, Arguments &&...)
	{
		return OperationResult::Unchanged;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Not &not_, Arguments &&... arguments)
	{
		return not_.argument.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	static OperationResult visit(ast::Or &or_, Arguments &&... arguments)
	{
		auto operationResult = OperationResult::Unchanged;

		for (auto &argument : or_.arguments)
			if (argument.accept(ForEachVariableDeclarationVisitor(), std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	template <class... Arguments>
	static OperationResult visit(ast::Predicate &, Arguments &&...)
	{
		return OperationResult::Unchanged;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CheckIfDefinitionFalseFunctor
{
	OperationResult operator()(ast::VariableDeclaration &variableDeclaration,
		ast::Formula &, ast::Formula &definition, VariableDomainMap &variableDomainMap)
	{
		if (variableDeclaration.domain != ast::Domain::Unknown)
			return OperationResult::Unchanged;

		clearVariableDomainMap(variableDomainMap);

		auto result = evaluate(definition, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
			return OperationResult::Unchanged;

		// As a hypothesis, make the parameter’s domain noninteger
		variableDomainMap[&variableDeclaration] = ast::Domain::General;

		result = evaluate(definition, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
		{
			// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
			variableDeclaration.domain = ast::Domain::Integer;
			return OperationResult::Changed;
		}

		return OperationResult::Unchanged;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CheckIfQuantifiedFormulaFalseFunctor
{
	OperationResult operator()(ast::VariableDeclaration &variableDeclaration,
		ast::Formula &quantifiedFormula, VariableDomainMap &variableDomainMap)
	{
		if (variableDeclaration.domain != ast::Domain::Unknown)
			return OperationResult::Unchanged;

		clearVariableDomainMap(variableDomainMap);

		auto result = evaluate(quantifiedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
			return OperationResult::Unchanged;

		// As a hypothesis, make the parameter’s domain noninteger
		variableDomainMap[&variableDeclaration] = ast::Domain::General;

		result = evaluate(quantifiedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
		{
			// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
			variableDeclaration.domain = ast::Domain::Integer;
			return OperationResult::Changed;
		}

		return OperationResult::Unchanged;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CheckIfCompletedFormulaTrueFunctor
{
	OperationResult operator()(ast::VariableDeclaration &variableDeclaration,
		ast::Formula &, ast::Formula &completedFormula, VariableDomainMap &variableDomainMap)
	{
		if (variableDeclaration.domain != ast::Domain::Unknown)
			return OperationResult::Unchanged;

		clearVariableDomainMap(variableDomainMap);

		auto result = evaluate(completedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::True)
			return OperationResult::Unchanged;

		// As a hypothesis, make the parameter’s domain noninteger
		variableDomainMap[&variableDeclaration] = ast::Domain::General;

		result = evaluate(completedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::True)
		{
			// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
			variableDeclaration.domain = ast::Domain::Integer;
			return OperationResult::Changed;
		}

		return OperationResult::Unchanged;
	}
};

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
			if (completedFormula.accept(ForEachVariableDeclarationVisitor<CheckIfQuantifiedFormulaFalseFunctor>(), variableDomainMap) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

			if (completedFormula.accept(ForEachVariableDeclarationVisitor<CheckIfCompletedFormulaTrueFunctor>(), completedFormula, variableDomainMap) == OperationResult::Changed)
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

			if (completedFormula.accept(ForEachVariableDeclarationVisitor<CheckIfDefinitionFalseFunctor>(), definition, variableDomainMap) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

			assert(predicate.arguments.size() == predicate.declaration->arity());

			for (size_t i = 0; i < predicate.arguments.size(); i++)
			{
				auto &variableArgument = predicate.arguments[i];
				auto &parameter = predicate.declaration->parameters[i];

				assert(variableArgument.is<ast::Variable>());

				auto &variable = variableArgument.get<ast::Variable>();

				parameter.domain = variable.declaration->domain;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
