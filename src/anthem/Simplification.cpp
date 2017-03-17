#include <anthem/Simplification.h>

#include <iostream>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bool isPrimitiveTerm(const ast::Term &term)
{
	const auto handleBinaryOperation =
		[](const ast::BinaryOperationPointer &)
		{
			return false;
		};

	const auto handleInterval =
		[](const ast::IntervalPointer &)
		{
			return false;
		};

	const auto handleDefault =
		[](const auto &)
		{
			return true;
		};

	return term.match(handleBinaryOperation, handleInterval, handleDefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matchesVariable(const ast::Term &term, const ast::VariablePointer &variable)
{
	const auto handleVariable =
		[&](const ast::VariablePointer &other)
		{
			return variable->name == other->name;
		};

	const auto handleDefault =
		[](const auto &)
		{
			return false;
		};

	return term.match(handleVariable, handleDefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::experimental::optional<ast::Term> extractAssignedTerm(ast::Formula &formula, const ast::VariablePointer &variable)
{
	const auto handleComparison =
		[&](ast::ComparisonPointer &comparison) -> std::experimental::optional<ast::Term>
		{
			if (comparison->operator_ != ast::Comparison::Operator::Equal)
				return std::experimental::nullopt;

			if (matchesVariable(comparison->left, variable))
				return std::move(comparison->right);

			if (matchesVariable(comparison->right, variable))
				return std::move(comparison->left);

			return std::experimental::nullopt;
		};

	const auto handleDefault =
		[](auto &) -> std::experimental::optional<ast::Term>
		{
			return std::experimental::nullopt;
		};

	return formula.match(handleComparison, handleDefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula simplify(ast::ExistsPointer &exists)
{
	if (!exists->argument.is<ast::AndPointer>())
		return std::move(exists);

	auto &conjunction = exists->argument.get_unchecked<ast::AndPointer>();
	auto &arguments = conjunction->arguments;

	// Check that formula is in normal form
	if (!arguments.back().is<ast::PredicatePointer>())
		return std::move(exists);

	const auto replaceVariableInPredicateWithTerm =
		[](ast::PredicatePointer &predicate, const ast::VariablePointer &variable, ast::Term &&term)
		{
			for (auto &argument : predicate->arguments)
			{
				if (!matchesVariable(argument, variable))
					continue;

				argument = std::move(term);
				break;
			}
		};

	// Simplify formulas of type “exists X (X = t and F(Y))” to “F(t)”
	for (auto i = exists->variables.begin(); i != exists->variables.end();)
	{
		auto &variable = *i;

		bool wasVariableReplaced = false;

		for (auto j = arguments.begin(); j != arguments.end(); j++)
		{
			auto &argument = *j;
			auto assignedTerm = extractAssignedTerm(argument, variable);

			if (!assignedTerm)
				continue;

			auto &lastArgument = arguments.back().get_unchecked<ast::PredicatePointer>();

			// If this argument is an assignment of the variable to some other term, remove the assignment and replace the variable with the other term
			replaceVariableInPredicateWithTerm(lastArgument, variable, std::move(assignedTerm.value()));

			arguments.erase(j);
			wasVariableReplaced = true;
			break;
		}

		if (wasVariableReplaced)
		{
			i = exists->variables.erase(i);
			continue;
		}

		i++;
	}

	if (exists->variables.empty())
	{
		assert(!conjunction->arguments.empty());

		if (conjunction->arguments.size() == 1)
			return std::move(conjunction->arguments.front());

		return std::move(exists->argument);
	}

	return std::move(exists);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula simplify(ast::Formula &&formula)
{
	const auto handleAnd =
		[&](ast::AndPointer &and_) -> ast::Formula
		{
			for (auto &argument : and_->arguments)
				argument = simplify(std::move(argument));

			return std::move(and_);
		};

	const auto handleBiconditional =
		[&](ast::BiconditionalPointer &biconditional) -> ast::Formula
		{
			biconditional->left = simplify(std::move(biconditional->left));
			biconditional->right = simplify(std::move(biconditional->right));

			return std::move(biconditional);
		};

	const auto handleExists =
		[&](ast::ExistsPointer &exists) -> ast::Formula
		{
			exists->argument = simplify(std::move(exists->argument));

			return simplify(exists);
		};

	const auto handleForAll =
		[&](ast::ForAllPointer &forAll) -> ast::Formula
		{
			forAll->argument = simplify(std::move(forAll->argument));

			return std::move(forAll);
		};

	const auto handleImplies =
		[&](ast::ImpliesPointer &implies) -> ast::Formula
		{
			implies->antecedent = simplify(std::move(implies->antecedent));
			implies->consequent = simplify(std::move(implies->consequent));

			return std::move(implies);
		};

	const auto handleIn =
		[](ast::InPointer &in) -> ast::Formula
		{
			if (!isPrimitiveTerm(in->element) || !isPrimitiveTerm(in->set))
				return std::move(in);

			// Simplify formulas of type “A in B” to “A = B” if A and B are primitive
			return std::make_unique<ast::Comparison>(ast::Comparison::Operator::Equal, std::move(in->element), std::move(in->set));
		};

	const auto handleNot =
		[&](ast::NotPointer &not_) -> ast::Formula
		{
			not_->argument = simplify(std::move(not_->argument));

			return std::move(not_);
		};

	const auto handleOr =
		[&](ast::OrPointer &or_) -> ast::Formula
		{
			for (auto &argument : or_->arguments)
				argument = simplify(std::move(argument));

			return std::move(or_);
		};

	const auto handleDefault =
		[&](auto &x) -> ast::Formula
		{
			return std::move(x);
		};

	return formula.match(handleAnd, handleBiconditional, handleExists, handleForAll, handleImplies, handleIn, handleNot, handleOr, handleDefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
