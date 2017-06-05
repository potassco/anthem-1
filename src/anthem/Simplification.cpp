#include <anthem/Simplification.h>

#include <experimental/optional>

#include <anthem/ASTCopy.h>
#include <anthem/ASTVisitors.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Determines whether a term is a specific variable
bool matchesVariableDeclaration(const ast::Term &term, const ast::VariableDeclaration &variableDeclaration)
{
	if (!term.is<ast::Variable>())
		return false;

	return term.get<ast::Variable>().declaration == &variableDeclaration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Extracts the term t if the given formula is of the form “X = t” and X matches the given variable
// The input formula is no longer usable after this call if a term is returned
std::experimental::optional<ast::Term> extractAssignedTerm(ast::Formula &formula, const ast::VariableDeclaration &variableDeclaration)
{
	if (!formula.is<ast::Comparison>())
		return std::experimental::nullopt;

	auto &comparison = formula.get<ast::Comparison>();

	if (comparison.operator_ != ast::Comparison::Operator::Equal)
		return std::experimental::nullopt;

	if (matchesVariableDeclaration(comparison.left, variableDeclaration))
		return std::move(comparison.right);

	if (matchesVariableDeclaration(comparison.right, variableDeclaration))
		return std::move(comparison.left);

	return std::experimental::nullopt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of a variable in a given term with another term
struct ReplaceVariableInTermVisitor : public ast::RecursiveTermVisitor<ReplaceVariableInTermVisitor>
{
	static void accept(ast::Variable &variable, ast::Term &term, const ast::VariableDeclaration &original, const ast::Term &replacement)
	{
		if (variable.declaration == &original)
			// No dangling variables can result from this operation, and hence, fixing them is not necessary
			term = ast::prepareCopy(replacement);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Term &, const ast::VariableDeclaration &, const ast::Term &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of a variable in a given formula with a term
struct ReplaceVariableInFormulaVisitor : public ast::RecursiveFormulaVisitor<ReplaceVariableInFormulaVisitor>
{
	static void accept(ast::Comparison &comparison, ast::Formula &, const ast::VariableDeclaration &original, const ast::Term &replacement)
	{
		comparison.left.accept(ReplaceVariableInTermVisitor(), comparison.left, original, replacement);
		comparison.right.accept(ReplaceVariableInTermVisitor(), comparison.right, original, replacement);
	}

	static void accept(ast::In &in, ast::Formula &, const ast::VariableDeclaration &original, const ast::Term &term)
	{
		in.element.accept(ReplaceVariableInTermVisitor(), in.element, original, term);
		in.set.accept(ReplaceVariableInTermVisitor(), in.set, original, term);
	}

	static void accept(ast::Predicate &predicate, ast::Formula &, const ast::VariableDeclaration &original, const ast::Term &replacement)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(ReplaceVariableInTermVisitor(), argument, original, replacement);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &, const ast::VariableDeclaration &, const ast::Term &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Simplifies exists statements by using the equivalence “exists X (X = t and F(X))” == “F(t)”
// The exists statement has to be of the form “exists <variables> <conjunction>”
void simplify(ast::Exists &exists, ast::Formula &formula)
{
	// Simplify formulas like “exists X (X = Y)” to “#true”
	// TODO: check that this covers all cases
	if (exists.argument.is<ast::Comparison>())
	{
		const auto &comparison = exists.argument.get<ast::Comparison>();

		if (comparison.operator_ != ast::Comparison::Operator::Equal)
			return;

		const auto matchingAssignment = std::find_if(exists.variables.cbegin(), exists.variables.cend(),
			[&](const auto &variableDeclaration)
			{
				return matchesVariableDeclaration(comparison.left, *variableDeclaration)
					|| matchesVariableDeclaration(comparison.right, *variableDeclaration);
			});

		if (matchingAssignment != exists.variables.cend())
			formula = ast::Formula::make<ast::Boolean>(true);

		return;
	}

	if (!exists.argument.is<ast::And>())
		return;

	auto &conjunction = exists.argument.get<ast::And>();
	auto &arguments = conjunction.arguments;

	// Simplify formulas of type “exists X (X = t and F(X))” to “F(t)”
	for (auto i = exists.variables.begin(); i != exists.variables.end();)
	{
		const auto &variableDeclaration = **i;

		bool wasVariableReplaced = false;

		// TODO: refactor
		for (auto j = arguments.begin(); j != arguments.end(); j++)
		{
			auto &argument = *j;
			// Find term that is equivalent to the given variable
			auto assignedTerm = extractAssignedTerm(argument, variableDeclaration);

			if (!assignedTerm)
				continue;

			// Replace all occurrences of the variable with the equivalent term
			for (auto k = arguments.begin(); k != arguments.end(); k++)
			{
				if (k == j)
					continue;

				auto &otherArgument = *k;
				otherArgument.accept(ReplaceVariableInFormulaVisitor(), otherArgument, variableDeclaration, assignedTerm.value());
			}

			arguments.erase(j);
			wasVariableReplaced = true;
			break;
		}

		if (wasVariableReplaced)
		{
			i = exists.variables.erase(i);
			continue;
		}

		i++;
	}

	// If there are still remaining variables, simplification is over
	if (!exists.variables.empty())
		return;

	assert(!arguments.empty());

	// If the argument now is a conjunction with just one element, directly replace the input formula with the argument
	if (arguments.size() == 1)
	{
		formula = std::move(arguments.front());
		return;
	}

	// If there is more than one element in the conjunction, replace the input formula with the conjunction
	formula = std::move(exists.argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Performs the different simplification techniques
struct SimplifyFormulaVisitor : public ast::RecursiveFormulaVisitor<SimplifyFormulaVisitor>
{
	// Forward exists statements to the dedicated simplification function
	static void accept(ast::Exists &exists, ast::Formula &formula)
	{
		simplify(exists, formula);
	}

	// Simplify formulas of type “A in B” to “A = B” if A and B are primitive
	static void accept(ast::In &in, ast::Formula &formula)
	{
		assert(ast::isPrimitive(in.element));

		if (!ast::isPrimitive(in.element) || !ast::isPrimitive(in.set))
			return;

		formula = ast::Comparison(ast::Comparison::Operator::Equal, std::move(in.element), std::move(in.set));
	}

	// Do nothing for all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void simplify(ast::Formula &formula)
{
	formula.accept(SimplifyFormulaVisitor(), formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
