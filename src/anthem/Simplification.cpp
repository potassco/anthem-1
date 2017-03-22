#include <anthem/Simplification.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bool isPrimitiveTerm(const ast::Term &term)
{
	return (!term.is<ast::BinaryOperation>() && !term.is<ast::Interval>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
struct RecursiveFormulaVisitor
{
	void visit(ast::And &and_, ast::Formula &formula)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, argument);

		return T::accept(and_, formula);
	}

	void visit(ast::Biconditional &biconditional, ast::Formula &formula)
	{
		biconditional.left.accept(*this, biconditional.left);
		biconditional.right.accept(*this, biconditional.right);

		return T::accept(biconditional, formula);
	}

	void visit(ast::Boolean &boolean, ast::Formula &formula)
	{
		return T::accept(boolean, formula);
	}

	void visit(ast::Comparison &comparison, ast::Formula &formula)
	{
		return T::accept(comparison, formula);
	}

	void visit(ast::Exists &exists, ast::Formula &formula)
	{
		exists.argument.accept(*this, exists.argument);

		return T::accept(exists, formula);
	}

	void visit(ast::ForAll &forAll, ast::Formula &formula)
	{
		forAll.argument.accept(*this, forAll.argument);

		return T::accept(forAll, formula);
	}

	void visit(ast::Implies &implies, ast::Formula &formula)
	{
		implies.antecedent.accept(*this, implies.antecedent);
		implies.consequent.accept(*this, implies.consequent);

		return T::accept(implies, formula);
	}

	void visit(ast::In &in, ast::Formula &formula)
	{
		return T::accept(in, formula);
	}

	void visit(ast::Not &not_, ast::Formula &formula)
	{
		not_.argument.accept(*this, not_.argument);

		return T::accept(not_, formula);
	}

	void visit(ast::Or &or_, ast::Formula &formula)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, argument);

		return T::accept(or_, formula);
	}

	void visit(ast::Predicate &predicate, ast::Formula &formula)
	{
		return T::accept(predicate, formula);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matchesVariable(const ast::Term &term, const ast::Variable &variable)
{
	if (!term.is<ast::Variable>())
		return false;

	const auto otherVariable = term.get<ast::Variable>();

	return variable.name == otherVariable.name;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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

void simplify(ast::Exists &exists, ast::Formula &formula)
{
	if (!exists.argument.is<ast::And>())
		return;

	auto &conjunction = exists.argument.get<ast::And>();
	auto &arguments = conjunction.arguments;

	// Check that formula is in normal form
	if (!arguments.back().is<ast::Predicate>())
		return;

	const auto replaceVariableInPredicateWithTerm =
		[](ast::Predicate &predicate, const ast::Variable &variable, ast::Term &term)
		{
			for (auto &argument : predicate.arguments)
			{
				if (!matchesVariable(argument, variable))
					continue;

				argument = std::move(term);
				break;
			}
		};

	// Simplify formulas of type “exists X (X = t and F(Y))” to “F(t)”
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

			auto &lastArgument = arguments.back().get<ast::Predicate>();

			// If this argument is an assignment of the variable to some other term, remove the assignment and replace the variable with the other term
			replaceVariableInPredicateWithTerm(lastArgument, variable, assignedTerm.value());

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

	// If there are still variables, do nothing more
	if (!exists.variables.empty())
		return;

	assert(!conjunction.arguments.empty());

	// If the argument is a conjunction with just one element, directly replace the input formula with the argument
	if (conjunction.arguments.size() == 1)
	{
		auto test = std::move(conjunction.arguments.front());
		formula = std::move(test);
		return;
	}

	// If there is more than one element in the conjunction, replace the input formula with the conjunction
	formula = std::move(exists.argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplifyFormulaVisitor : public RecursiveFormulaVisitor<SimplifyFormulaVisitor>
{
	static void accept(ast::Exists &exists, ast::Formula &formula)
	{
		simplify(exists, formula);
	}

	static void accept(ast::In &in, ast::Formula &formula)
	{
		if (!isPrimitiveTerm(in.element) || !isPrimitiveTerm(in.set))
			return;

		// Simplify formulas of type “A in B” to “A = B” if A and B are primitive
		formula = ast::Comparison(ast::Comparison::Operator::Equal, std::move(in.element), std::move(in.set));
	}

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
