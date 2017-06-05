#include <anthem/Completion.h>

#include <anthem/AST.h>
#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Exception.h>
#include <anthem/HiddenPredicateElimination.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Completion
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Builds the conjunction within the completed formula for a given predicate
ast::Formula buildCompletedFormulaDisjunction(const ast::Predicate &predicate, const ast::VariableDeclarationPointers &parameters, std::vector<ast::ScopedFormula> &scopedFormulas)
{
	auto disjunction = ast::Formula::make<ast::Or>();

	assert(predicate.arguments.size() == parameters.size());

	// Build the disjunction of all formulas with the predicate as consequent
	for (auto &scopedFormula : scopedFormulas)
	{
		assert(scopedFormula.formula.is<ast::Implies>());
		auto &implies = scopedFormula.formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
			continue;

		auto &otherPredicate = implies.consequent.get<ast::Predicate>();

		if (!ast::matches(predicate, otherPredicate))
			continue;

		assert(otherPredicate.arguments.size() == parameters.size());

		// Each formula with the predicate as its consequent currently has its own copy of the predicate’s parameters
		// These need to be linked to the new, unique set of parameters
		for (size_t i = 0; i < parameters.size(); i++)
		{
			assert(otherPredicate.arguments[i].is<ast::Variable>());
			const auto &otherVariable = otherPredicate.arguments[i].get<ast::Variable>();

			scopedFormula.formula.accept(ast::ReplaceVariableInFormulaVisitor(), scopedFormula.formula, otherVariable.declaration, parameters[i].get());
		}

		// Remove all the head variables, because they are not free variables after completion
		const auto isHeadVariable =
			[](const auto &variableDeclaration)
			{
				return variableDeclaration->type == ast::VariableDeclaration::Type::Head;
			};

		auto &freeVariables = scopedFormula.freeVariables;
		freeVariables.erase(std::remove_if(freeVariables.begin(), freeVariables.end(), isHeadVariable), freeVariables.end());

		if (freeVariables.empty())
			disjunction.get<ast::Or>().arguments.emplace_back(std::move(implies.antecedent));
		else
		{
			auto exists = ast::Formula::make<ast::Exists>(std::move(freeVariables), std::move(implies.antecedent));
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

ast::Formula completePredicate(const ast::PredicateSignature &predicateSignature, std::vector<ast::ScopedFormula> &scopedFormulas)
{
	// Create new set of parameters for the completed definition for the predicate
	ast::VariableDeclarationPointers parameters;
	parameters.reserve(predicateSignature.arity);

	std::vector<ast::Term> arguments;
	arguments.reserve(predicateSignature.arity);

	for (size_t i = 0; i < predicateSignature.arity; i++)
	{
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head));
		arguments.emplace_back(ast::Term::make<ast::Variable>(parameters.back().get()));
	}

	ast::Predicate predicateCopy(std::string(predicateSignature.name), std::move(arguments));

	auto completedFormulaDisjunction = buildCompletedFormulaDisjunction(predicateCopy, parameters, scopedFormulas);
	auto completedFormulaQuantified = buildCompletedFormulaQuantified(std::move(predicateCopy), std::move(completedFormulaDisjunction));

	if (parameters.empty())
		return completedFormulaQuantified;

	return ast::Formula::make<ast::ForAll>(std::move(parameters), std::move(completedFormulaQuantified));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula completeIntegrityConstraint(ast::ScopedFormula &scopedFormula)
{
	assert(scopedFormula.formula.is<ast::Implies>());
	auto &implies = scopedFormula.formula.get<ast::Implies>();
	assert(implies.consequent.is<ast::Boolean>());
	assert(implies.consequent.get<ast::Boolean>().value == false);

	auto argument = ast::Formula::make<ast::Not>(std::move(implies.antecedent));

	auto &freeVariables = scopedFormula.freeVariables;

	if (freeVariables.empty())
		return argument;

	return ast::Formula::make<ast::ForAll>(std::move(freeVariables), std::move(argument));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::Formula> complete(std::vector<ast::ScopedFormula> &&scopedFormulas, Context &context)
{
	// Check whether formulas are in normal form
	for (const auto &scopedFormula : scopedFormulas)
	{
		if (!scopedFormula.formula.is<ast::Implies>())
			throw CompletionException("cannot perform completion, formula not in normal form");

		auto &implies = scopedFormula.formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>() && !implies.consequent.is<ast::Boolean>())
			throw CompletionException("cannot perform completion, only single predicates and Booleans supported as formula consequent currently");
	}

	std::vector<ast::PredicateSignature> predicateSignatures;

	// Get a list of all predicates
	for (const auto &scopedFormula : scopedFormulas)
		ast::collectPredicateSignatures(scopedFormula.formula, predicateSignatures);

	std::sort(predicateSignatures.begin(), predicateSignatures.end(),
		[](const auto &lhs, const auto &rhs)
		{
			const auto order = std::strcmp(lhs.name.c_str(), rhs.name.c_str());

			if (order != 0)
				return (order < 0);

			return lhs.arity < rhs.arity;
		});

	std::vector<ast::Formula> completedFormulas;

	// Complete predicates
	for (const auto &predicateSignature : predicateSignatures)
		completedFormulas.emplace_back(completePredicate(predicateSignature, scopedFormulas));

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

		completedFormulas.emplace_back(completeIntegrityConstraint(scopedFormula));
	}

	// Eliminate all predicates that should not be visible in the output
	eliminateHiddenPredicates(predicateSignatures, completedFormulas, context);

	return completedFormulas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
