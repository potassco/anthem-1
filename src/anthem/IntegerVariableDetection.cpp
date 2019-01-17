#include <anthem/IntegerVariableDetection.h>

#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Evaluation.h>
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

EvaluationResult evaluate(const ast::Formula &formula, VariableDomainMap &variableDomainMap)
{
	return evaluate<VariableDomainMap>(formula, variableDomainMap);
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

		// As a hypothesis, make the parameter symbolic
		variableDomainMap[&variableDeclaration] = Domain::Symbolic;

		const auto result = evaluate(definition, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
		{
			// If making the variable symbolic leads to a false or erroneous result, it’s proven to be integer
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

		// As a hypothesis, make the parameter symbolic
		variableDomainMap[&variableDeclaration] = Domain::Symbolic;

		const auto result = evaluate(quantifiedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::False)
		{
			// If making the variable symbolic leads to a false or erroneous result, it’s proven to be integer
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

		// As a hypothesis, make the parameter symbolic
		variableDomainMap[&variableDeclaration] = Domain::Symbolic;

		const auto result = evaluate(completedFormula, variableDomainMap);

		if (result == EvaluationResult::Error || result == EvaluationResult::True)
		{
			// If making the variable symbolic leads to a false or erroneous result, it’s proven to be integer
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
