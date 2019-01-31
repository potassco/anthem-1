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
	ast::Or or_;

	assert(predicate.arguments.size() == parameters.size());

	// Build the disjunction of all formulas with the predicate as consequent
	for (auto &scopedFormula : scopedFormulas)
	{
		assert(scopedFormula.formula.is<ast::Implies>());
		auto &implies = scopedFormula.formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
			continue;

		auto &otherPredicate = implies.consequent.get<ast::Predicate>();

		if (predicate.declaration != otherPredicate.declaration)
			continue;

		assert(otherPredicate.arguments.size() == parameters.size());

		auto &freeVariables = scopedFormula.freeVariables;

		// Each formula with the predicate as its consequent currently has its own copy of the predicate’s parameters
		// These need to be linked to the new, unique set of parameters

		// First, remove the free variables whose occurrences will be relinked, which is why they are no longer needed
		const auto isFreeVariableUnneeded =
			[&](const auto &freeVariable)
			{
				const auto matchesVariableToBeReplaced = std::find_if(otherPredicate.arguments.cbegin(), otherPredicate.arguments.cend(),
					[&](const ast::Term &argument)
					{
						assert(argument.is<ast::Variable>());
						const auto &otherVariable = argument.get<ast::Variable>();

						return (freeVariable.get() == otherVariable.declaration);
					});

				return (matchesVariableToBeReplaced != otherPredicate.arguments.cend());
			};

		freeVariables.erase(std::remove_if(freeVariables.begin(), freeVariables.end(), isFreeVariableUnneeded), freeVariables.end());

		// Currently, only rules with singleton heads are supported
		// Rules with multiple elements in the head are not yet handled correctly by the head variable detection mechanism
		for (const auto &freeVariable : freeVariables)
			if (freeVariable->type == ast::VariableDeclaration::Type::Head)
				throw CompletionException("cannot perform completion, only singleton rule heads supported currently");

		// Second, link all occurrences of the deleted free variable to the new, unique parameter
		for (size_t i = 0; i < parameters.size(); i++)
		{
			assert(otherPredicate.arguments[i].is<ast::Variable>());
			const auto &otherVariable = otherPredicate.arguments[i].get<ast::Variable>();

			scopedFormula.formula.accept(ast::ReplaceVariableInFormulaVisitor(), scopedFormula.formula, otherVariable.declaration, parameters[i].get());
		}

		if (freeVariables.empty())
			or_.arguments.emplace_back(std::move(implies.antecedent));
		else
		{
			ast::Exists exists(std::move(freeVariables), std::move(implies.antecedent));
			or_.arguments.emplace_back(std::move(exists));
		}
	}

	return std::move(or_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Builds the quantified inner part of the completed formula
ast::Formula buildCompletedFormulaQuantified(ast::Predicate &&predicate, ast::Formula &&innerFormula)
{
	assert(innerFormula.is<ast::Or>());

	if (innerFormula.get<ast::Or>().arguments.empty())
		return ast::Not(std::move(predicate));

	if (innerFormula.get<ast::Or>().arguments.size() == 1)
		innerFormula = std::move(innerFormula.get<ast::Or>().arguments.front());

	if (innerFormula.is<ast::Boolean>())
	{
		const auto &boolean = innerFormula.get<ast::Boolean>();

		if (boolean.value == true)
			return std::move(predicate);
		else
			return ast::Not(std::move(predicate));
	}

	return ast::Biconditional(std::move(predicate), std::move(innerFormula));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula completePredicate(ast::PredicateDeclaration &predicateDeclaration, std::vector<ast::ScopedFormula> &scopedFormulas)
{
	// Create new set of parameters for the completed definition for the predicate
	ast::VariableDeclarationPointers parameters;
	parameters.reserve(predicateDeclaration.arity());

	std::vector<ast::Term> arguments;
	arguments.reserve(predicateDeclaration.arity());

	for (size_t i = 0; i < predicateDeclaration.arity(); i++)
	{
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head));
		arguments.emplace_back(ast::Variable(parameters.back().get()));
	}

	ast::Predicate predicateCopy(&predicateDeclaration, std::move(arguments));

	auto completedFormulaDisjunction = buildCompletedFormulaDisjunction(predicateCopy, parameters, scopedFormulas);
	auto completedFormulaQuantified = buildCompletedFormulaQuantified(std::move(predicateCopy), std::move(completedFormulaDisjunction));

	if (parameters.empty())
		return completedFormulaQuantified;

	return ast::ForAll(std::move(parameters), std::move(completedFormulaQuantified));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula completeIntegrityConstraint(ast::ScopedFormula &scopedFormula)
{
	assert(scopedFormula.formula.is<ast::Implies>());
	auto &implies = scopedFormula.formula.get<ast::Implies>();
	assert(implies.consequent.is<ast::Boolean>());
	assert(implies.consequent.get<ast::Boolean>().value == false);

	auto argument = ast::Not(std::move(implies.antecedent));

	auto &freeVariables = scopedFormula.freeVariables;

	if (freeVariables.empty())
		return std::move(argument);

	return ast::ForAll(std::move(freeVariables), std::move(argument));
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

	std::sort(context.predicateDeclarations.begin(), context.predicateDeclarations.end(),
		[](const auto &lhs, const auto &rhs)
		{
			const auto order = std::strcmp(lhs->name.c_str(), rhs->name.c_str());

			if (order != 0)
				return (order < 0);

			return lhs->arity() < rhs->arity();
		});

	std::vector<ast::Formula> completedFormulas;

	// Complete predicates
	for (auto &predicateDeclaration : context.predicateDeclarations)
	{
		if (!predicateDeclaration->isUsed || predicateDeclaration->isExternal)
			continue;

		completedFormulas.emplace_back(completePredicate(*predicateDeclaration, scopedFormulas));
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

		completedFormulas.emplace_back(completeIntegrityConstraint(scopedFormula));
	}

	// Eliminate all predicates that should not be visible in the output
	eliminateHiddenPredicates(completedFormulas, context);

	return completedFormulas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
