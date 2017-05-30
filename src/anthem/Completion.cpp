	#include <anthem/Completion.h>

#include <anthem/AST.h>
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
std::vector<std::unique_ptr<ast::VariableDeclaration>> copyParameters(const ast::Predicate &predicate)
{
	std::vector<std::unique_ptr<ast::VariableDeclaration>> parameters;
	/*parameters.reserve(predicate.arity());

	for (const auto &argument : predicate.arguments)
	{
		assert(argument.is<ast::Variable>());
		// TODO: reimplement
		//parameters.emplace_back(ast::deepCopy(parameter.get<ast::VariableDeclaration>()));
	}*/

	return parameters;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Builds the conjunction within the completed formula for a given predicate
ast::Formula buildCompletedFormulaDisjunction(const ast::Predicate &predicate, std::vector<std::unique_ptr<ast::VariableDeclaration>> &parameters, std::vector<ast::ScopedFormula> &scopedFormulas)
{
	auto disjunction = ast::Formula::make<ast::Or>();

	/*ast::VariableStack variableStack;
	variableStack.push(&parameters);

	// Build the conjunction of all formulas with the predicate as consequent
	for (auto &scopedFormula : scopedFormulas)
	{
		assert(scopedFormula.formula.is<ast::Implies>());
		auto &implies = scopedFormula.formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
			continue;

		auto &otherPredicate = implies.consequent.get<ast::Predicate>();

		if (!ast::matches(predicate, otherPredicate))
			continue;

		auto variables = ast::collectFreeVariables(implies.antecedent, variableStack);

		// TODO: avoid deep copies
		// TODO: reimplement
		if (variables.empty())
			disjunction.get<ast::Or>().arguments.emplace_back(ast::deepCopy(implies.antecedent));
		else
		{
			auto exists = ast::Formula::make<ast::Exists>(std::move(variables), ast::deepCopy(implies.antecedent));
			disjunction.get<ast::Or>().arguments.emplace_back(std::move(exists));
		}
	}*/

	return disjunction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Builds the quantified inner part of the completed formula
ast::Formula buildCompletedFormulaQuantified(ast::Predicate &&predicate, ast::Formula &&innerFormula)
{
	assert(innerFormula.is<ast::Or>());

	/*if (innerFormula.get<ast::Or>().arguments.empty())
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
	}*/

	return ast::Formula::make<ast::Biconditional>(std::move(predicate), std::move(innerFormula));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void completePredicate(ast::Predicate &&predicate, std::vector<ast::ScopedFormula> &scopedFormulas, std::vector<ast::ScopedFormula> &completedScopedFormulas)
{
	/*auto parameters = copyParameters(predicate);
	auto completedFormulaDisjunction = buildCompletedFormulaDisjunction(predicate, parameters, scopedFormulas);
	auto completedFormulaQuantified = buildCompletedFormulaQuantified(std::move(predicate), std::move(completedFormulaDisjunction));

	if (parameters.empty())
	{
		completedFormulas.emplace_back(std::move(completedFormulaQuantified));
		return;
	}

	auto completedFormula = ast::Formula::make<ast::ForAll>(std::move(parameters), std::move(completedFormulaQuantified));
	completedFormulas.emplace_back(std::move(completedFormula));*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void completeIntegrityConstraint(ast::Formula &formula, std::vector<ast::ScopedFormula> &)
{
	/*assert(formula.is<ast::Implies>());
	auto &implies = formula.get<ast::Implies>();
	assert(implies.consequent.is<ast::Boolean>());
	assert(implies.consequent.get<ast::Boolean>().value == false);

	auto variables = ast::collectFreeVariables(implies.antecedent);

	// TODO: avoid deep copies
	// TODO: reimplement
	auto argument = ast::Formula::make<ast::Not>(ast::deepCopy(implies.antecedent));

	if (variables.empty())
	{
		completedFormulas.emplace_back(std::move(argument));
		return;
	}

	auto completedFormula = ast::Formula::make<ast::ForAll>(std::move(variables), std::move(argument));
	completedFormulas.emplace_back(std::move(completedFormula));*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void complete(std::vector<ast::ScopedFormula> &scopedFormulas)
{
	/*// Check whether formulas are in normal form
	for (const auto &scopedFormula : scopedFormulas)
	{
		if (!scopedFormula.formula.is<ast::Implies>())
			throw std::runtime_error("cannot perform completion, formula not in normal form");

		auto &implies = scopedFormula.formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>() && !implies.consequent.is<ast::Boolean>())
			throw std::runtime_error("cannot perform completion, only single predicates and Booleans supported as formula consequent currently");
	}

	std::vector<const ast::Predicate *> predicates;

	for (const auto &scopedFormula : scopedFormulas)
		ast::collectPredicates(scopedFormula.formula, predicates);

	std::sort(predicates.begin(), predicates.end(),
		[](const auto *lhs, const auto *rhs)
		{
			const auto order = std::strcmp(lhs->name.c_str(), rhs->name.c_str());

			if (order != 0)
				return order < 0;

			return lhs->arity() < rhs->arity();
		});

	std::vector<ast::ScopedFormula> completedScopedFormulas;

	// Complete predicates
	for (const auto *predicate : predicates)
	{
		// Create the signature of the predicate
		ast::Predicate signature(std::string(predicate->name));
		signature.arguments.reserve(predicate->arguments.size());

		// TODO: reimplement
		for (std::size_t i = 0; i < predicate->arguments.size(); i++)
		{
			auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(i + 1);
			signature.arguments.emplace_back(ast::Term::make<ast::Variable>(std::move(variableName), ast::VariableDeclaration::Type::Reserved));
		}

		completePredicate(std::move(signature), scopedFormulas, completedScopedFormulas);
	}

	// Complete integrity constraints
	for (auto &scopedFormula : scopedFormulas)
	{
		auto &implies = scopedFormula.formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Boolean>())
			continue;

		const auto &boolean = implies.consequent.get<ast::Boolean>();

		// Rules of the form “F -> #true” are useless
		if (boolean.value == true)
			continue;

		completeIntegrityConstraint(scopedFormula.formula, completedScopedFormulas);
	}

	std::swap(scopedFormulas, completedScopedFormulas);*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
