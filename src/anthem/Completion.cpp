#include <anthem/Completion.h>

#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Completion
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Checks that two matching predicates (same name, same arity) have the same arguments
void checkMatchingPredicates(const ast::Term &lhs, const ast::Term &rhs)
{
	if (!lhs.is<ast::Variable>() || !rhs.is<ast::Variable>())
		throw std::runtime_error("cannot preform completion, only variables supported in predicates currently");

	if (lhs.get<ast::Variable>().name != rhs.get<ast::Variable>().name)
		throw std::runtime_error("cannot perform completion, inconsistent predicate argument naming");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void completePredicate(const ast::Predicate &predicate, std::vector<ast::Formula> &formulas, std::size_t formulasBegin)
{
	// Check that predicate is in normal form
	for (auto i = formulasBegin; i < formulas.size(); i++)
	{
		auto &formula = formulas[i];
		assert(formula.is<ast::Implies>());
		auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
			continue;

		auto &otherPredicate = implies.consequent.get<ast::Predicate>();

		if (predicate.arity() != otherPredicate.arity() || predicate.name != otherPredicate.name)
			continue;

		for (std::size_t i = 0; i < predicate.arguments.size(); i++)
			checkMatchingPredicates(predicate.arguments[i], otherPredicate.arguments[i]);
	}

	// Copy the predicate’s arguments for the completed formula
	std::vector<ast::Variable> variables;
	variables.reserve(predicate.arguments.size());

	for (const auto &argument : predicate.arguments)
	{
		assert(argument.is<ast::Variable>());
		variables.emplace_back(ast::deepCopy(argument.get<ast::Variable>()));
	}

	auto or_ = ast::Formula::make<ast::Or>();

	// Build the conjunction of all formulas with the predicate as consequent
	for (auto i = formulasBegin; i < formulas.size();)
	{
		auto &formula = formulas[i];
		assert(formula.is<ast::Implies>());
		auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
		{
			i++;
			continue;
		}

		auto &otherPredicate = implies.consequent.get<ast::Predicate>();

		if (predicate.arity() != otherPredicate.arity() || predicate.name != otherPredicate.name)
		{
			i++;
			continue;
		}

		ast::VariableStack variableStack;
		variableStack.push(&variables);

		auto variables = ast::collectFreeVariables(implies.antecedent, variableStack);

		if (variables.empty())
			or_.get<ast::Or>().arguments.emplace_back(std::move(implies.antecedent));
		else
		{
			auto exists = ast::Formula::make<ast::Exists>(std::move(variables), std::move(implies.antecedent));
			or_.get<ast::Or>().arguments.emplace_back(std::move(exists));
		}

		if (i > formulasBegin)
			formulas.erase(formulas.begin() + i);
		else
			i++;
	}

	if (or_.get<ast::Or>().arguments.size() == 1)
		or_ = or_.get<ast::Or>().arguments.front();

	// Build the biconditional within the completed formula
	auto biconditional = ast::Formula::make<ast::Biconditional>(ast::deepCopy(predicate), std::move(or_));

	if (predicate.arguments.empty())
	{
		formulas[formulasBegin] = std::move(biconditional);
		return;
	}

	auto completedFormula = ast::Formula::make<ast::ForAll>(std::move(variables), std::move(biconditional));

	formulas[formulasBegin] = std::move(completedFormula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void completeBoolean(ast::Formula &formula)
{
	assert(formula.is<ast::Implies>());
	auto &implies = formula.get<ast::Implies>();
	assert(implies.consequent.is<ast::Boolean>());
	auto &boolean = implies.consequent.get<ast::Boolean>();

	auto variables = ast::collectFreeVariables(implies.antecedent);

	auto argument = (boolean.value == true)
		? std::move(implies.antecedent)
		: ast::Formula::make<ast::Not>(std::move(implies.antecedent));

	if (variables.empty())
	{
		formula = std::move(argument);
		return;
	}

	formula = ast::Formula::make<ast::ForAll>(std::move(variables), std::move(argument));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void complete(std::vector<ast::Formula> &formulas)
{
	for (const auto &formula : formulas)
	{
		if (!formula.is<ast::Implies>())
			throw std::runtime_error("cannot perform completion, formula not in normal form");

		auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>() && !implies.consequent.is<ast::Boolean>())
			throw std::runtime_error("cannot perform completion, only single predicates and Booleans supported as formula consequent currently");
	}

	for (std::size_t i = 0; i < formulas.size(); i++)
	{
		auto &formula = formulas[i];
		auto &implies = formula.get<ast::Implies>();

		if (implies.consequent.is<ast::Predicate>())
		{
			auto &predicate = implies.consequent.get<ast::Predicate>();
			completePredicate(predicate, formulas, i);
		}
		else if (implies.consequent.is<ast::Boolean>())
			completeBoolean(formula);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
