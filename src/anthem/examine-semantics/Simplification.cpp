#include <anthem/examine-semantics/Simplification.h>

#include <optional>

#include <anthem/ASTCopy.h>
#include <anthem/Equality.h>
#include <anthem/examine-semantics/SimplificationVisitors.h>
#include <anthem/examine-semantics/Type.h>

namespace anthem
{
namespace examineSemantics
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
OperationResult simplify(ast::Formula &formula)
{
	return SimplificationRule::apply(formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class FirstSimplificationRule, class SecondSimplificationRule, class... OtherSimplificationRules>
OperationResult simplify(ast::Formula &formula)
{
	if (simplify<FirstSimplificationRule>(formula) == OperationResult::Changed)
		return OperationResult::Changed;

	return simplify<SecondSimplificationRule, OtherSimplificationRules...>(formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleExistsWithoutQuantifiedVariables
{
	static constexpr const auto Description = "exists () (F) === F";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return OperationResult::Unchanged;

		auto &exists = formula.get<ast::Exists>();

		if (!exists.variables.empty())
			return OperationResult::Unchanged;

		formula = std::move(exists.argument);

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleTrivialAssignmentInExists
{
	static constexpr const auto Description = "exists X (X = Y) === #true";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return OperationResult::Unchanged;

		const auto &exists = formula.get<ast::Exists>();

		if (!exists.argument.is<ast::Comparison>())
			return OperationResult::Unchanged;

		const auto &comparison = exists.argument.get<ast::Comparison>();

		if (comparison.operator_ != ast::Comparison::Operator::Equal)
			return OperationResult::Unchanged;

		const auto matchingAssignment = std::find_if(exists.variables.cbegin(), exists.variables.cend(),
			[&](const auto &variableDeclaration)
			{
				return matchesVariableDeclaration(comparison.left, *variableDeclaration)
					|| matchesVariableDeclaration(comparison.right, *variableDeclaration);
			});

		if (matchingAssignment == exists.variables.cend())
			return OperationResult::Unchanged;

		formula = ast::Boolean(true);

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleAssignmentInExists
{
	static constexpr const auto Description = "exists X (X = t and F(X)) === exists () (F(t))";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return OperationResult::Unchanged;

		auto &exists = formula.get<ast::Exists>();

		if (!exists.argument.is<ast::And>())
			return OperationResult::Unchanged;

		auto &and_ = exists.argument.get<ast::And>();
		auto &arguments = and_.arguments;

		auto simplificationResult = OperationResult::Unchanged;

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
				simplificationResult = OperationResult::Changed;

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

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::And>())
			return OperationResult::Unchanged;

		auto &and_ = formula.get<ast::And>();

		if (!and_.arguments.empty())
			return OperationResult::Unchanged;

		formula = ast::Boolean(true);

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleOneElementConjunction
{
	static constexpr const auto Description = "[conjunction of only F] === F";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::And>())
			return OperationResult::Unchanged;

		auto &and_ = formula.get<ast::And>();

		if (and_.arguments.size() != 1)
			return OperationResult::Unchanged;

		formula = std::move(and_.arguments.front());

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleTrivialExists
{
	static constexpr const auto Description = "exists ... ([#true/#false]) === [#true/#false]";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Exists>())
			return OperationResult::Unchanged;

		auto &exists = formula.get<ast::Exists>();

		if (!exists.argument.is<ast::Boolean>())
			return OperationResult::Unchanged;

		formula = std::move(exists.argument);

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleInWithPrimitiveArguments
{
	static constexpr const auto Description = "[primitive A] in [primitive B] === A = B";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::In>())
			return OperationResult::Unchanged;

		auto &in = formula.get<ast::In>();

		assert(ast::isPrimitive(in.element));

		if (!ast::isPrimitive(in.element) || !ast::isPrimitive(in.set))
			return OperationResult::Unchanged;

		formula = ast::Comparison(ast::Comparison::Operator::Equal, std::move(in.element), std::move(in.set));

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleSubsumptionInBiconditionals
{
	static constexpr const auto Description = "(F <-> (F and G)) === (F -> G)";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Biconditional>())
			return OperationResult::Unchanged;

		auto &biconditional = formula.get<ast::Biconditional>();

		const auto leftIsPredicate = biconditional.left.is<ast::Predicate>();
		const auto rightIsPredicate = biconditional.right.is<ast::Predicate>();

		const auto leftIsAnd = biconditional.left.is<ast::And>();
		const auto rightIsAnd = biconditional.right.is<ast::And>();

		if (!(leftIsPredicate && rightIsAnd) && !(rightIsPredicate && leftIsAnd))
			return OperationResult::Unchanged;

		auto &predicateSide = (leftIsPredicate ? biconditional.left : biconditional.right);
		auto &andSide = (leftIsPredicate ? biconditional.right : biconditional.left);
		auto &and_ = andSide.get<ast::And>();

		const auto matchingPredicate =
			std::find_if(and_.arguments.cbegin(), and_.arguments.cend(),
			[&](const auto &argument)
			{
				return (ast::equal(predicateSide, argument) == Tristate::True);
			});

		if (matchingPredicate == and_.arguments.cend())
			return OperationResult::Unchanged;

		and_.arguments.erase(matchingPredicate);

		formula = ast::Implies(std::move(predicateSide), std::move(andSide));

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleDoubleNegation
{
	static constexpr const auto Description = "not not F === F";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Not>())
			return OperationResult::Unchanged;

		auto &not_ = formula.get<ast::Not>();

		if (!not_.argument.is<ast::Not>())
			return OperationResult::Unchanged;

		auto &notNot = not_.argument.get<ast::Not>();

		formula = std::move(notNot.argument);

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleDeMorganForConjunctions
{
	static constexpr const auto Description = "(not (F and G)) === (not F or not G)";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Not>())
			return OperationResult::Unchanged;

		auto &not_ = formula.get<ast::Not>();

		if (!not_.argument.is<ast::And>())
			return OperationResult::Unchanged;

		auto &and_ = not_.argument.get<ast::And>();

		for (auto &argument : and_.arguments)
			argument = ast::Not(std::move(argument));

		formula = ast::Or(std::move(and_.arguments));

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleImplicationFromDisjunction
{
	static constexpr const auto Description = "(not F or G) === (F -> G)";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Or>())
			return OperationResult::Unchanged;

		auto &or_ = formula.get<ast::Or>();

		if (or_.arguments.size() != 2)
			return OperationResult::Unchanged;

		const auto leftIsNot = or_.arguments[0].is<ast::Not>();
		const auto rightIsNot = or_.arguments[1].is<ast::Not>();

		if (leftIsNot == rightIsNot)
			return OperationResult::Unchanged;

		auto &negativeSide = leftIsNot ? or_.arguments[0] : or_.arguments[1];
		auto &positiveSide = leftIsNot ? or_.arguments[1] : or_.arguments[0];

		assert(negativeSide.is<ast::Not>());
		assert(!positiveSide.is<ast::Not>());

		auto &negativeSideArgument = negativeSide.get<ast::Not>().argument;

		formula = ast::Implies(std::move(negativeSideArgument), std::move(positiveSide));

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleNegatedComparison
{
	static constexpr const auto Description = "(not F [comparison] G) === (F [negated comparison] G)";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::Not>())
			return OperationResult::Unchanged;

		auto &not_ = formula.get<ast::Not>();

		if (!not_.argument.is<ast::Comparison>())
			return OperationResult::Unchanged;

		auto &comparison = not_.argument.get<ast::Comparison>();

		switch (comparison.operator_)
		{
			case ast::Comparison::Operator::GreaterThan:
				comparison.operator_ = ast::Comparison::Operator::LessEqual;
				break;
			case ast::Comparison::Operator::LessThan:
				comparison.operator_ = ast::Comparison::Operator::GreaterEqual;
				break;
			case ast::Comparison::Operator::LessEqual:
				comparison.operator_ = ast::Comparison::Operator::GreaterThan;
				break;
			case ast::Comparison::Operator::GreaterEqual:
				comparison.operator_ = ast::Comparison::Operator::LessThan;
				break;
			case ast::Comparison::Operator::NotEqual:
				comparison.operator_ = ast::Comparison::Operator::Equal;
				break;
			case ast::Comparison::Operator::Equal:
				comparison.operator_ = ast::Comparison::Operator::NotEqual;
				break;
		}

		formula = std::move(comparison);

		return OperationResult::Changed;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SimplificationRuleIntegerSetInclusion
{
	static constexpr const auto Description = "(F in G) === (F = G) if F and G are integer variables";

	static OperationResult apply(ast::Formula &formula)
	{
		if (!formula.is<ast::In>())
			return OperationResult::Unchanged;

		auto &in = formula.get<ast::In>();

		const auto elementType = type(in.element);
		const auto setType = type(in.set);

		if (elementType.domain != Domain::Integer || setType.domain != Domain::Integer
		    || elementType.setSize != SetSize::Unit || setType.setSize != SetSize::Unit)
		{
			return OperationResult::Unchanged;
		}

		formula = ast::Comparison(ast::Comparison::Operator::Equal, std::move(in.element), std::move(in.set));

		return OperationResult::Changed;
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
		SimplificationRuleSubsumptionInBiconditionals,
		SimplificationRuleDeMorganForConjunctions,
		SimplificationRuleImplicationFromDisjunction,
		SimplificationRuleNegatedComparison,
		SimplificationRuleIntegerSetInclusion
	>;

////////////////////////////////////////////////////////////////////////////////////////////////////

// Performs the different simplification techniques
struct SimplifyFormulaVisitor : public FormulaSimplificationVisitor<SimplifyFormulaVisitor>
{
	// Do nothing for all other types of expressions
	static OperationResult accept(ast::Formula &formula)
	{
		return simplifyWithDefaultRules(formula);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void simplify(ast::Formula &formula)
{
	while (formula.accept(SimplifyFormulaVisitor(), formula) == OperationResult::Changed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
