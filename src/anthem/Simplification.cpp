#include <anthem/Simplification.h>

#include <anthem/ASTVisitors.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Determins whether a term is primitive
// All terms but binary operations and interval are primitive
// With primitive terms t, “X in t” and “X = t” are equivalent
bool isPrimitiveTerm(const ast::Term &term)
{
	return (!term.is<ast::BinaryOperation>() && !term.is<ast::Interval>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Determines whether a term is a specific variable
bool matchesVariable(const ast::Term &term, const ast::Variable &variable)
{
	if (!term.is<ast::Variable>())
		return false;

	const auto otherVariable = term.get<ast::Variable>();

	return variable.name == otherVariable.name;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Extracts the term t if the given formula is of the form “X = t” and X matches the given variable
// The input formula is not usable if a term is returned
std::experimental::optional<ast::Term> extractAssignedTerm(ast::Formula &formula, const ast::Variable &variable)
{
	if (!formula.is<ast::Comparison>())
		return std::experimental::nullopt;

	auto &comparison = formula.get<ast::Comparison>();

	if (comparison.operator_ != ast::Comparison::Operator::Equal)
		return std::experimental::nullopt;

	if (matchesVariable(comparison.left, variable))
		return std::move(comparison.right);

	if (matchesVariable(comparison.right, variable))
		return std::move(comparison.left);

	return std::experimental::nullopt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces the first occurrence of a variable with a term in a given formula
struct ReplaceVariableWithTermVisitor : public ast::RecursiveFormulaVisitor<ReplaceVariableWithTermVisitor>
{
	// Perform replacement in predicates only
	static void accept(ast::Predicate &predicate, ast::Formula &, const ast::Variable &variable, ast::Term &&term)
	{
		for (auto &argument : predicate.arguments)
		{
			if (!matchesVariable(argument, variable))
				continue;

			argument = std::move(term);
			return;
		}
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &, const ast::Variable &, ast::Term &&)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Simplifies exists statements by using the equivalence “exists X (X = t and F(X))” == “F(t)”
// The exists statement has to be of the form “exists <variables> <conjunction>”
void simplify(ast::Exists &exists, ast::Formula &formula)
{
	if (!exists.argument.is<ast::And>())
		return;

	auto &conjunction = exists.argument.get<ast::And>();
	auto &arguments = conjunction.arguments;

	// Check that formula is in normal form
	if (!arguments.back().is<ast::Predicate>() && !arguments.back().is<ast::Not>())
		return;

	// Simplify formulas of type “exists X (X = t and F(X))” to “F(t)”
	for (auto i = exists.variables.begin(); i != exists.variables.end();)
	{
		auto &variable = *i;

		bool wasVariableReplaced = false;

		for (auto j = arguments.begin(); j != arguments.end(); j++)
		{
			auto &argument = *j;
			auto assignedTerm = extractAssignedTerm(argument, variable);

			if (!assignedTerm)
				continue;

			// If this argument is an assignment of the variable to some other term, remove the assignment and replace the variable with the other term
			arguments.back().accept(ReplaceVariableWithTermVisitor(), arguments.back(), variable, std::move(assignedTerm.value()));

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

	assert(!conjunction.arguments.empty());

	// If the argument now is a conjunction with just one element, directly replace the input formula with the argument
	if (conjunction.arguments.size() == 1)
	{
		// TODO: remove workaround
		auto tmp = std::move(conjunction.arguments.front());
		formula = std::move(tmp);
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
		if (!isPrimitiveTerm(in.element) || !isPrimitiveTerm(in.set))
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
