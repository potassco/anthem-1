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
	template <class... Arguments>
	void visit(ast::And &and_, ast::Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(and_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Biconditional &biconditional, ast::Formula &formula, Arguments &&... arguments)
	{
		biconditional.left.accept(*this, biconditional.left, std::forward<Arguments>(arguments)...);
		biconditional.right.accept(*this, biconditional.right, std::forward<Arguments>(arguments)...);

		return T::accept(biconditional, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Boolean &boolean, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(boolean, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Comparison &comparison, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(comparison, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Exists &exists, ast::Formula &formula, Arguments &&... arguments)
	{
		exists.argument.accept(*this, exists.argument, std::forward<Arguments>(arguments)...);

		return T::accept(exists, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::ForAll &forAll, ast::Formula &formula, Arguments &&... arguments)
	{
		forAll.argument.accept(*this, forAll.argument, std::forward<Arguments>(arguments)...);

		return T::accept(forAll, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Implies &implies, ast::Formula &formula, Arguments &&... arguments)
	{
		implies.antecedent.accept(*this, implies.antecedent, std::forward<Arguments>(arguments)...);
		implies.consequent.accept(*this, implies.consequent, std::forward<Arguments>(arguments)...);

		return T::accept(implies, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::In &in, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(in, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Not &not_, ast::Formula &formula, Arguments &&... arguments)
	{
		not_.argument.accept(*this, not_.argument, std::forward<Arguments>(arguments)...);

		return T::accept(not_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Or &or_, ast::Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(or_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Predicate &predicate, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(predicate, formula, std::forward<Arguments>(arguments)...);
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

struct ReplaceVariableWithTermVisitor : public RecursiveFormulaVisitor<ReplaceVariableWithTermVisitor>
{
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

	template<class T>
	static void accept(T &, ast::Formula &, const ast::Variable &, ast::Term &&)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void simplify(ast::Exists &exists, ast::Formula &formula)
{
	if (!exists.argument.is<ast::And>())
		return;

	auto &conjunction = exists.argument.get<ast::And>();
	auto &arguments = conjunction.arguments;

	// Check that formula is in normal form
	if (!arguments.back().is<ast::Predicate>() && !arguments.back().is<ast::Not>())
		return;

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

	// If there are still variables, do nothing more
	if (!exists.variables.empty())
		return;

	assert(!conjunction.arguments.empty());

	// If the argument is a conjunction with just one element, directly replace the input formula with the argument
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
