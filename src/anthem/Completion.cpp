#include <anthem/Completion.h>

#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Completion
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Copies the parameters of a predicate
std::vector<ast::Variable> copyParameters(const ast::Predicate &predicate)
{
	std::vector<ast::Variable> parameters;
	parameters.reserve(predicate.arity());

	for (const auto &parameter : predicate.arguments)
	{
		assert(parameter.is<ast::Variable>());
		parameters.emplace_back(ast::deepCopy(parameter.get<ast::Variable>()));
	}

	return parameters;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Builds the conjunction within the completed formula for a given predicate
ast::Formula buildCompletedFormulaDisjunction(const ast::Predicate &predicate, const std::vector<ast::Variable> &parameters, const std::vector<ast::Formula> &formulas)
{
	auto disjunction = ast::Formula::make<ast::Or>();

	ast::VariableStack variableStack;
	variableStack.push(&parameters);

	// Build the conjunction of all formulas with the predicate as consequent
	for (const auto &formula : formulas)
	{
		assert(formula.is<ast::Implies>());
		const auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
			continue;

		auto &otherPredicate = implies.consequent.get<ast::Predicate>();

		if (!ast::matches(predicate, otherPredicate))
			continue;

		auto variables = ast::collectFreeVariables(implies.antecedent, variableStack);

		// TODO: avoid deep copies
		if (variables.empty())
			disjunction.get<ast::Or>().arguments.emplace_back(ast::deepCopy(implies.antecedent));
		else
		{
			auto exists = ast::Formula::make<ast::Exists>(std::move(variables), ast::deepCopy(implies.antecedent));
			disjunction.get<ast::Or>().arguments.emplace_back(std::move(exists));
		}
	}

	return disjunction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Builds the quantified inner part of the completed formula
ast::Formula buildCompletedFormulaQuantified(ast::Predicate &&predicate, ast::Formula &&innerFormula)
{
	assert(innerFormula.is<ast::Or>());

	if (innerFormula.get<ast::Or>().arguments.empty())
		return ast::Formula::make<ast::Not>(std::move(predicate));

	if (innerFormula.get<ast::Or>().arguments.size() == 1)
		innerFormula = std::move(innerFormula.get<ast::Or>().arguments.front());

	if (innerFormula.is<ast::Boolean>())
	{
		const auto &boolean = innerFormula.get<ast::Boolean>();

		if (boolean.value == true)
			return std::move(predicate);
		else
			return ast::Formula::make<ast::Not>(std::move(predicate));
	}

	return ast::Formula::make<ast::Biconditional>(std::move(predicate), std::move(innerFormula));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void completePredicate(ast::Predicate &&predicate, const std::vector<ast::Formula> &formulas, std::vector<ast::Formula> &completedFormulas)
{
	auto parameters = copyParameters(predicate);
	auto completedFormulaDisjunction = buildCompletedFormulaDisjunction(predicate, parameters, formulas);
	auto completedFormulaQuantified = buildCompletedFormulaQuantified(std::move(predicate), std::move(completedFormulaDisjunction));

	if (parameters.empty())
	{
		completedFormulas.emplace_back(std::move(completedFormulaQuantified));
		return;
	}

	auto completedFormula = ast::Formula::make<ast::ForAll>(std::move(parameters), std::move(completedFormulaQuantified));
	completedFormulas.emplace_back(std::move(completedFormula));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void completeIntegrityConstraint(const ast::Formula &formula, std::vector<ast::Formula> &completedFormulas)
{
	assert(formula.is<ast::Implies>());
	auto &implies = formula.get<ast::Implies>();
	assert(implies.consequent.is<ast::Boolean>());
	assert(implies.consequent.get<ast::Boolean>().value == false);

	auto variables = ast::collectFreeVariables(implies.antecedent);

	// TODO: avoid deep copies
	auto argument = ast::Formula::make<ast::Not>(ast::deepCopy(implies.antecedent));

	if (variables.empty())
	{
		completedFormulas.emplace_back(std::move(argument));
		return;
	}

	auto completedFormula = ast::Formula::make<ast::ForAll>(std::move(variables), std::move(argument));
	completedFormulas.emplace_back(std::move(completedFormula));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void complete(std::vector<ast::Formula> &formulas)
{
	// Check whether formulas are in normal form
	for (const auto &formula : formulas)
	{
		if (!formula.is<ast::Implies>())
			throw std::runtime_error("cannot perform completion, formula not in normal form");

		auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>() && !implies.consequent.is<ast::Boolean>())
			throw std::runtime_error("cannot perform completion, only single predicates and Booleans supported as formula consequent currently");
	}

	std::vector<const ast::Predicate *> predicates;

	for (const auto &formula : formulas)
		ast::collectPredicates(formula, predicates);

	std::sort(predicates.begin(), predicates.end(),
		[](const auto *lhs, const auto *rhs)
		{
			const auto order = std::strcmp(lhs->name.c_str(), rhs->name.c_str());

			if (order != 0)
				return order < 0;

			return lhs->arity() < rhs->arity();
		});

	std::vector<ast::Formula> completedFormulas;

	// Complete predicates
	for (const auto *predicate : predicates)
	{
		// Create the signature of the predicate
		ast::Predicate signature(std::string(predicate->name));
		signature.arguments.reserve(predicate->arguments.size());

		for (std::size_t i = 0; i < predicate->arguments.size(); i++)
		{
			auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(i + 1);
			signature.arguments.emplace_back(ast::Term::make<ast::Variable>(std::move(variableName), ast::Variable::Type::Reserved));
		}

		completePredicate(std::move(signature), formulas, completedFormulas);
	}

	// Complete integrity constraints
	for (const auto &formula : formulas)
	{
		auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Boolean>())
			continue;

		const auto &boolean = implies.consequent.get<ast::Boolean>();

		// Rules of the form “F -> #true” are useless
		if (boolean.value == true)
			continue;

		completeIntegrityConstraint(formula, completedFormulas);
	}

	std::swap(formulas, completedFormulas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
