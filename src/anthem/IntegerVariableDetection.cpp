#include <anthem/IntegerVariableDetection.h>

#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Exception.h>
#include <anthem/Simplification.h>
#include <anthem/Type.h>
#include <anthem/Utils.h>
#include <anthem/output/AST.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IntegerVariableDetection
//
////////////////////////////////////////////////////////////////////////////////////////////////////

using VariableDomainMap = std::map<const ast::VariableDeclaration *, Domain>;

////////////////////////////////////////////////////////////////////////////////////////////////////

Domain domain(const ast::Variable &variable, VariableDomainMap &variableDomainMap)
{
	if (variable.declaration->domain != Domain::Unknown)
		return variable.declaration->domain;

	const auto match = variableDomainMap.find(variable.declaration);

	if (match == variableDomainMap.end())
		return Domain::Unknown;

	return match->second;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void clearVariableDomainMap(VariableDomainMap &variableDomainMap)
{
	for (auto &variableDeclaration : variableDomainMap)
		variableDeclaration.second = Domain::Unknown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct VariableDomainMapAccessor
{
	Domain operator()(const ast::Variable &variable, VariableDomainMap &variableDomainMap)
	{
		return domain(variable, variableDomainMap);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

Type type(const ast::Term &term, VariableDomainMap &variableDomainMap)
{
	return type<VariableDomainMapAccessor>(term, variableDomainMap);
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
		const auto leftType = type(comparison.left, variableDomainMap);
		const auto rightType = type(comparison.right, variableDomainMap);

		// Comparisons with empty sets always return false
		if (leftType.setSize == SetSize::Empty || rightType.setSize == SetSize::Empty)
			return EvaluationResult::False;

		// If either side has an unknown domain, the result is unknown
		if (leftType.domain == Domain::Unknown || rightType.domain == Domain::Unknown)
			return EvaluationResult::Unknown;

		// If both sides have the same domain, the result is unknown
		if (leftType.domain == rightType.domain)
			return EvaluationResult::Unknown;

		// If one side is integer, but the other one isn’t, they are not equal
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
		const auto elementType = type(in.element, variableDomainMap);
		const auto setType = type(in.set, variableDomainMap);

		// The element to test shouldn’t be empty or a proper set by itself
		assert(elementType.setSize != SetSize::Empty && elementType.setSize != SetSize::Multi);

		// If the set is empty, no element can be selected
		if (setType.setSize == SetSize::Empty)
			return EvaluationResult::False;

		// If one of the sides has an unknown type, the result is unknown
		if (elementType.domain == Domain::Unknown || setType.domain == Domain::Unknown)
			return EvaluationResult::Unknown;

		// If both sides have the same domain, the result is unknown
		if (elementType.domain == setType.domain)
			return EvaluationResult::Unknown;

		// If one side is integer, but the other one isn’t, set inclusion is never satisfied
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

			if (parameter.domain != Domain::Integer)
				continue;

			const auto argumentType = type(argument, variableDomainMap);

			if (argumentType.domain == Domain::Noninteger || argumentType.setSize == SetSize::Empty)
				return EvaluationResult::Error;
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
		if (variableDeclaration.domain != Domain::Unknown)
			return OperationResult::Unchanged;

		clearVariableDomainMap(variableDomainMap);

		// As a hypothesis, make the parameter’s domain noninteger
		variableDomainMap[&variableDeclaration] = Domain::Noninteger;

		const auto result = evaluate(definition, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
		{
			// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
			variableDeclaration.domain = Domain::Integer;
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
		if (variableDeclaration.domain != Domain::Unknown)
			return OperationResult::Unchanged;

		clearVariableDomainMap(variableDomainMap);

		// As a hypothesis, make the parameter’s domain noninteger
		variableDomainMap[&variableDeclaration] = Domain::Noninteger;

		const auto result = evaluate(quantifiedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
		{
			// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
			variableDeclaration.domain = Domain::Integer;
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
		if (variableDeclaration.domain != Domain::Unknown)
			return OperationResult::Unchanged;

		clearVariableDomainMap(variableDomainMap);

		// As a hypothesis, make the parameter’s domain noninteger
		variableDomainMap[&variableDeclaration] = Domain::Noninteger;

		const auto result = evaluate(completedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::True)
		{
			// If making the variable noninteger leads to a false or erroneous result, it’s proven to be integer
			variableDeclaration.domain = Domain::Integer;
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
