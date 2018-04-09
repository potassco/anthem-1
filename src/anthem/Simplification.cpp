#include <anthem/Simplification.h>

#include <optional>

#include <anthem/ASTCopy.h>
#include <anthem/Equality.h>
#include <anthem/output/AST.h>
#include <anthem/SimplificationVisitors.h>

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
std::optional<ast::Term> extractAssignedTerm(ast::Formula &formula, const ast::VariableDeclaration &variableDeclaration)
{
	if (!formula.is<ast::Comparison>())
		return std::nullopt;

	auto &comparison = formula.get<ast::Comparison>();

	if (comparison.operator_ != ast::Comparison::Operator::Equal)
		return std::nullopt;

	if (matchesVariableDeclaration(comparison.left, variableDeclaration))
		return std::move(comparison.right);

	if (matchesVariableDeclaration(comparison.right, variableDeclaration))
		return std::move(comparison.left);

	return std::nullopt;
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

template<class SimplificationRule>
SimplificationResult simplify(ast::Formula &formula)
{
	return SimplificationRule::apply(formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class FirstSimplificationRule, class SecondSimplificationRule, class... OtherSimplificationRules>
SimplificationResult simplify(ast::Formula &formula)
{
	if (simplify<FirstSimplificationRule>(formula) == SimplificationResult::Simplified)
		return SimplificationResult::Simplified;

	return simplify<SecondSimplificationRule, OtherSimplificationRules...>(formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleExistsWithoutQuantifiedVariables
{
	static constexpr const auto Description = "exists () (F) === F";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return SimplificationResult::Unchanged;

		auto &exists = formula.get<ast::Exists>();

		if (!exists.variables.empty())
			return SimplificationResult::Unchanged;

		formula = std::move(exists.argument);

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleTrivialAssignmentInExists
{
	static constexpr const auto Description = "exists X (X = Y) === #true";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return SimplificationResult::Unchanged;

		const auto &exists = formula.get<ast::Exists>();

		if (!exists.argument.is<ast::Comparison>())
			return SimplificationResult::Unchanged;

		const auto &comparison = exists.argument.get<ast::Comparison>();

		if (comparison.operator_ != ast::Comparison::Operator::Equal)
			return SimplificationResult::Unchanged;

		const auto matchingAssignment = std::find_if(exists.variables.cbegin(), exists.variables.cend(),
			[&](const auto &variableDeclaration)
			{
				return matchesVariableDeclaration(comparison.left, *variableDeclaration)
					|| matchesVariableDeclaration(comparison.right, *variableDeclaration);
			});

		if (matchingAssignment == exists.variables.cend())
			return SimplificationResult::Unchanged;

		formula = ast::Formula::make<ast::Boolean>(true);

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleAssignmentInExists
{
	static constexpr const auto Description = "exists X (X = t and F(X)) === exists () (F(t))";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return SimplificationResult::Unchanged;

		auto &exists = formula.get<ast::Exists>();

		if (!exists.argument.is<ast::And>())
			return SimplificationResult::Unchanged;

		auto &and_ = exists.argument.get<ast::And>();
		auto &arguments = and_.arguments;

		auto simplificationResult = SimplificationResult::Unchanged;

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
				simplificationResult = SimplificationResult::Simplified;

				break;
			}

			if (wasVariableReplaced)
			{
				i = exists.variables.erase(i);
				continue;
			}

			i++;
		}

		return simplificationResult;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleEmptyConjunction
{
	static constexpr const auto Description = "[empty conjunction] === #true";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::And>())
			return SimplificationResult::Unchanged;

		auto &and_ = formula.get<ast::And>();

		if (!and_.arguments.empty())
			return SimplificationResult::Unchanged;

		formula = ast::Formula::make<ast::Boolean>(true);

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleOneElementConjunction
{
	static constexpr const auto Description = "[conjunction of only F] === F";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::And>())
			return SimplificationResult::Unchanged;

		auto &and_ = formula.get<ast::And>();

		if (and_.arguments.size() != 1)
			return SimplificationResult::Unchanged;

		formula = std::move(and_.arguments.front());

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleTrivialExists
{
	static constexpr const auto Description = "exists ... ([#true/#false]) === [#true/#false]";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return SimplificationResult::Unchanged;

		auto &exists = formula.get<ast::Exists>();

		if (!exists.argument.is<ast::Boolean>())
			return SimplificationResult::Unchanged;

		formula = std::move(exists.argument);

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleInWithPrimitiveArguments
{
	static constexpr const auto Description = "[primitive A] in [primitive B] === A = B";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::In>())
			return SimplificationResult::Unchanged;

		auto &in = formula.get<ast::In>();

		assert(ast::isPrimitive(in.element));

		if (!ast::isPrimitive(in.element) || !ast::isPrimitive(in.set))
			return SimplificationResult::Unchanged;

		formula = ast::Comparison(ast::Comparison::Operator::Equal, std::move(in.element), std::move(in.set));

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleSubsumptionInBiconditionals
{
	static constexpr const auto Description = "(F <-> (F and G)) === (F -> G)";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Biconditional>())
			return SimplificationResult::Unchanged;

		auto &biconditional = formula.get<ast::Biconditional>();

		const auto leftIsPredicate = biconditional.left.is<ast::Predicate>();
		const auto rightIsPredicate = biconditional.right.is<ast::Predicate>();

		const auto leftIsAnd = biconditional.left.is<ast::And>();
		const auto rightIsAnd = biconditional.right.is<ast::And>();

		if (!(leftIsPredicate && rightIsAnd) && !(rightIsPredicate && leftIsAnd))
			return SimplificationResult::Unchanged;

		auto &predicateSide = (leftIsPredicate ? biconditional.left : biconditional.right);
		auto &andSide = (leftIsPredicate ? biconditional.right : biconditional.left);
		auto &and_ = andSide.get<ast::And>();

		const auto matchingPredicate =
			std::find_if(and_.arguments.cbegin(), and_.arguments.cend(),
			[&](const auto &argument)
			{
				return (ast::equal(predicateSide, argument) == ast::Tristate::True);
			});

		if (matchingPredicate == and_.arguments.cend())
			return SimplificationResult::Unchanged;

		and_.arguments.erase(matchingPredicate);

		formula = ast::Formula::make<ast::Implies>(std::move(predicateSide), std::move(andSide));

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleDoubleNegation
{
	static constexpr const auto Description = "not not F === F";

	static SimplificationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Not>())
			return SimplificationResult::Unchanged;

		auto &not_ = formula.get<ast::Not>();

		if (!not_.argument.is<ast::Not>())
			return SimplificationResult::Unchanged;

		auto &notNot = not_.argument.get<ast::Not>();

		formula = std::move(notNot.argument);

		return SimplificationResult::Simplified;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto simplifyWithDefaultRules =
	simplify
	<
		SimplificationRuleDoubleNegation,
		SimplificationRuleTrivialAssignmentInExists,
		SimplificationRuleAssignmentInExists,
		SimplificationRuleEmptyConjunction,
		SimplificationRuleTrivialExists,
		SimplificationRuleOneElementConjunction,
		SimplificationRuleExistsWithoutQuantifiedVariables,
		SimplificationRuleInWithPrimitiveArguments,
		SimplificationRuleSubsumptionInBiconditionals
	>;

////////////////////////////////////////////////////////////////////////////////////////////////////

// Performs the different simplification techniques
struct SimplifyFormulaVisitor : public ast::FormulaSimplificationVisitor<SimplifyFormulaVisitor>
{
	// Do nothing for all other types of expressions
	static SimplificationResult accept(ast::Formula &formula)
	{
		return simplifyWithDefaultRules(formula);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void simplify(ast::Formula &formula)
{
	while (formula.accept(SimplifyFormulaVisitor(), formula) == SimplificationResult::Simplified);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
