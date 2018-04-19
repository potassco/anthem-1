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

ast::Domain domain(ast::Term &term);

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class OperationResult
{
	Unchanged,
	Changed,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermDomainVisitor
{
	static ast::Domain visit(ast::BinaryOperation &binaryOperation)
	{
		const auto leftDomain = domain(binaryOperation.left);
		const auto rightDomain = domain(binaryOperation.right);

		if (leftDomain == ast::Domain::General || rightDomain == ast::Domain::General)
			return ast::Domain::General;

		if (leftDomain == ast::Domain::Integer || rightDomain == ast::Domain::Integer)
			return ast::Domain::Integer;

		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::Boolean &)
	{
		return ast::Domain::General;
	}

	static ast::Domain visit(ast::Function &function)
	{
		return function.declaration->domain;
	}

	static ast::Domain visit(ast::Integer &)
	{
		return ast::Domain::Integer;
	}

	static ast::Domain visit(ast::Interval &interval)
	{
		const auto fromDomain = domain(interval.from);
		const auto toDomain = domain(interval.to);

		if (fromDomain == ast::Domain::General || toDomain == ast::Domain::General)
			return ast::Domain::General;

		if (fromDomain == ast::Domain::Integer || toDomain == ast::Domain::Integer)
			return ast::Domain::Integer;

		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::SpecialInteger &)
	{
		// TODO: check correctness
		return ast::Domain::Integer;
	}

	static ast::Domain visit(ast::String &)
	{
		return ast::Domain::General;
	}

	static ast::Domain visit(ast::UnaryOperation &unaryOperation)
	{
		return domain(unaryOperation.argument);
	}

	static ast::Domain visit(ast::Variable &variable)
	{
		return variable.declaration->domain;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Domain domain(ast::Term &term)
{
	return term.accept(TermDomainVisitor());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool isVariable(const ast::Term &term, const ast::VariableDeclaration &variableDeclaration)
{
	if (!term.is<ast::Variable>())
		return false;

	auto &variable = term.get<ast::Variable>();

	return (variable.declaration == &variableDeclaration);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct VariableDomainInFormulaVisitor
{
	static ast::Domain visit(ast::And &and_, ast::VariableDeclaration &variableDeclaration)
	{
		bool integer = false;

		for (auto &argument : and_.arguments)
		{
			const auto domain = argument.accept(VariableDomainInFormulaVisitor(), variableDeclaration);

			if (domain == ast::Domain::General)
				return ast::Domain::General;

			if (domain == ast::Domain::Integer)
				integer = true;
		}

		if (integer)
			return ast::Domain::Integer;

		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::Biconditional &biconditional, ast::VariableDeclaration &variableDeclaration)
	{
		const auto leftDomain = biconditional.left.accept(VariableDomainInFormulaVisitor(), variableDeclaration);
		const auto rightDomain = biconditional.right.accept(VariableDomainInFormulaVisitor(), variableDeclaration);

		if (leftDomain == ast::Domain::General || rightDomain == ast::Domain::General)
			return ast::Domain::General;

		if (leftDomain == ast::Domain::Integer || rightDomain == ast::Domain::Integer)
			return ast::Domain::Integer;

		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::Boolean &, ast::VariableDeclaration &)
	{
		// Variable doesn’t occur in Booleans, hence it’s still considered integer until the contrary is found
		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::Comparison &comparison, ast::VariableDeclaration &variableDeclaration)
	{
		const auto leftIsVariable = isVariable(comparison.left, variableDeclaration);
		const auto rightIsVariable = isVariable(comparison.right, variableDeclaration);

		// TODO: implement more cases
		if (!leftIsVariable && !rightIsVariable)
			return ast::Domain::Unknown;

		auto &otherSide = (leftIsVariable ? comparison.right : comparison.left);

		return domain(otherSide);
	}

	static ast::Domain visit(ast::Exists &exists, ast::VariableDeclaration &variableDeclaration)
	{
		return exists.argument.accept(VariableDomainInFormulaVisitor(), variableDeclaration);
	}

	static ast::Domain visit(ast::ForAll &forAll, ast::VariableDeclaration &variableDeclaration)
	{
		return forAll.argument.accept(VariableDomainInFormulaVisitor(), variableDeclaration);
	}

	static ast::Domain visit(ast::Implies &implies, ast::VariableDeclaration &variableDeclaration)
	{
		const auto antecedentDomain = implies.antecedent.accept(VariableDomainInFormulaVisitor(), variableDeclaration);
		const auto consequentDomain = implies.antecedent.accept(VariableDomainInFormulaVisitor(), variableDeclaration);

		if (antecedentDomain == ast::Domain::General || consequentDomain == ast::Domain::General)
			return ast::Domain::General;

		if (antecedentDomain == ast::Domain::Integer || consequentDomain == ast::Domain::Integer)
			return ast::Domain::Integer;

		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::In &in, ast::VariableDeclaration &variableDeclaration)
	{
		const auto elementIsVariable = isVariable(in.element, variableDeclaration);
		const auto setIsVariable = isVariable(in.set, variableDeclaration);

		// TODO: implement more cases
		if (!elementIsVariable && !setIsVariable)
			return ast::Domain::Unknown;

		auto &otherSide = (elementIsVariable ? in.set : in.element);

		return domain(otherSide);
	}

	static ast::Domain visit(ast::Not &not_, ast::VariableDeclaration &variableDeclaration)
	{
		return not_.argument.accept(VariableDomainInFormulaVisitor(), variableDeclaration);
	}

	static ast::Domain visit(ast::Or &or_, ast::VariableDeclaration &variableDeclaration)
	{
		bool integer = false;

		for (auto &argument : or_.arguments)
		{
			const auto domain = argument.accept(VariableDomainInFormulaVisitor(), variableDeclaration);

			if (domain == ast::Domain::General)
				return ast::Domain::General;

			if (domain == ast::Domain::Integer)
				integer = true;
		}

		if (integer)
			return ast::Domain::Integer;

		return ast::Domain::Unknown;
	}

	static ast::Domain visit(ast::Predicate &predicate, ast::VariableDeclaration &variableDeclaration)
	{
		// TODO: check implementation for nested arguments

		// Inherit the domain of the predicate’s parameters
		for (size_t i = 0; i < predicate.arguments.size(); i++)
		{
			auto &argument = predicate.arguments[i];

			if (!argument.is<ast::Variable>())
				continue;

			auto &variable = argument.get<ast::Variable>();

			if (variable.declaration != &variableDeclaration)
				continue;

			auto &parameter = predicate.declaration->parameters[i];

			return parameter.domain;
		}

		return ast::Domain::Unknown;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Recursively finds every variable declaration and executes functor to the formula in the scope of the declaration
struct DetectIntegerVariablesVisitor
{
	static OperationResult visit(ast::And &and_)
	{
		auto operationResult = OperationResult::Unchanged;

		for (auto &argument : and_.arguments)
			if (argument.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::Biconditional &biconditional)
	{
		auto operationResult = OperationResult::Unchanged;

		if (biconditional.left.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		if (biconditional.right.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::Boolean &)
	{
		return OperationResult::Unchanged;
	}

	static OperationResult visit(ast::Comparison &)
	{
		return OperationResult::Unchanged;
	}

	static OperationResult visit(ast::Exists &exists)
	{
		auto operationResult = OperationResult::Unchanged;

		if (exists.argument.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		for (auto &variableDeclaration : exists.variables)
			if (variableDeclaration->domain != ast::Domain::General)
			{
				auto newDomain = exists.argument.accept(VariableDomainInFormulaVisitor(), *variableDeclaration);

				if (variableDeclaration->domain == newDomain)
					continue;

				operationResult = OperationResult::Changed;
				variableDeclaration->domain = newDomain;
			}

		return operationResult;
	}

	static OperationResult visit(ast::ForAll &forAll)
	{
		auto operationResult = OperationResult::Unchanged;

		if (forAll.argument.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		for (auto &variableDeclaration : forAll.variables)
			if (variableDeclaration->domain != ast::Domain::General)
			{
				auto newDomain = forAll.argument.accept(VariableDomainInFormulaVisitor(), *variableDeclaration);

				if (variableDeclaration->domain == newDomain)
					continue;

				operationResult = OperationResult::Changed;
				variableDeclaration->domain = newDomain;
			}

		return operationResult;
	}

	static OperationResult visit(ast::Implies &implies)
	{
		auto operationResult = OperationResult::Unchanged;

		if (implies.antecedent.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		if (implies.consequent.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
			operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::In &)
	{
		return OperationResult::Unchanged;
	}

	static OperationResult visit(ast::Not &not_)
	{
		return not_.argument.accept(DetectIntegerVariablesVisitor());
	}

	static OperationResult visit(ast::Or &or_)
	{
		auto operationResult = OperationResult::Unchanged;

		for (auto &argument : or_.arguments)
			if (argument.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

		return operationResult;
	}

	static OperationResult visit(ast::Predicate &)
	{
		return OperationResult::Unchanged;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Assumes the completed formulas to be in translated but not simplified form.
// That is, completed formulas are either variable-free or universally quantified
void detectIntegerVariables(std::vector<ast::Formula> &completedFormulas)
{
	auto operationResult{OperationResult::Changed};

	while (operationResult == OperationResult::Changed)
	{
		operationResult = OperationResult::Unchanged;

		for (auto &completedFormula : completedFormulas)
		{
			if (!completedFormula.is<ast::ForAll>())
				continue;

			auto &forAll = completedFormula.get<ast::ForAll>();

			// TODO: check that integrity constraints are also handled
			if (!forAll.argument.is<ast::Biconditional>())
				continue;

			auto &biconditional = forAll.argument.get<ast::Biconditional>();

			if (!biconditional.left.is<ast::Predicate>())
				continue;

			auto &predicate = biconditional.left.get<ast::Predicate>();
			auto &definition = biconditional.right;

			if (definition.accept(DetectIntegerVariablesVisitor()) == OperationResult::Changed)
				operationResult = OperationResult::Changed;

			for (auto &variableDeclaration : forAll.variables)
				if (variableDeclaration->domain != ast::Domain::General)
				{
					auto newDomain = forAll.argument.accept(VariableDomainInFormulaVisitor(), *variableDeclaration);

					if (variableDeclaration->domain == newDomain)
						continue;

					operationResult = OperationResult::Changed;
					variableDeclaration->domain = newDomain;
				}

			assert(predicate.arguments.size() == predicate.declaration->arity());

			// Update parameter domains
			for (size_t i = 0; i < predicate.arguments.size(); i++)
			{
				auto &variableArgument = predicate.arguments[i];

				assert(variableArgument.is<ast::Variable>());

				auto &variable = variableArgument.get<ast::Variable>();
				auto &parameter = predicate.declaration->parameters[i];

				if (parameter.domain == variable.declaration->domain)
					continue;

				operationResult = OperationResult::Changed;
				parameter.domain = variable.declaration->domain;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
