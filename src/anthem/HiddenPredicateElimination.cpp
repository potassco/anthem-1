#include <anthem/HiddenPredicateElimination.h>

#include <anthem/ASTCopy.h>
#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>
#include <anthem/Exception.h>
#include <anthem/Simplification.h>
#include <anthem/output/AST.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HiddenPredicateElimination
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct PredicateReplacement
{
	const ast::Predicate &predicate;
	ast::Formula replacement;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of a variable in a given term with another variable
struct ReplaceVariableInTermVisitor : public ast::RecursiveTermVisitor<ReplaceVariableInTermVisitor>
{
	static void accept(ast::Variable &variable, ast::Term &, const ast::VariableDeclaration &original, ast::VariableDeclaration &replacement)
	{
		if (variable.declaration == &original)
			// No dangling variables can result from this operation, and hence, fixing them is not necessary
			variable.declaration = &replacement;
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Term &, const ast::VariableDeclaration &, ast::VariableDeclaration &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of a variable in a given formula with another variable
struct ReplaceVariableInFormulaVisitor : public ast::RecursiveFormulaVisitor<ReplaceVariableInFormulaVisitor>
{
	static void accept(ast::Comparison &comparison, ast::Formula &, const ast::VariableDeclaration &original, ast::VariableDeclaration &replacement)
	{
		comparison.left.accept(ReplaceVariableInTermVisitor(), comparison.left, original, replacement);
		comparison.right.accept(ReplaceVariableInTermVisitor(), comparison.right, original, replacement);
	}

	static void accept(ast::In &in, ast::Formula &, const ast::VariableDeclaration &original, ast::VariableDeclaration &replacement)
	{
		in.element.accept(ReplaceVariableInTermVisitor(), in.element, original, replacement);
		in.set.accept(ReplaceVariableInTermVisitor(), in.set, original, replacement);
	}

	static void accept(ast::Predicate &predicate, ast::Formula &, const ast::VariableDeclaration &original, ast::VariableDeclaration &replacement)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(ReplaceVariableInTermVisitor(), argument, original, replacement);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &, const ast::VariableDeclaration &, ast::VariableDeclaration &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replace a predicate in a term with a formula
struct ReplacePredicateInFormulaVisitor : public ast::RecursiveFormulaVisitor<ReplacePredicateInFormulaVisitor>
{
	static void accept(ast::Predicate &predicate, ast::Formula &formula, const PredicateReplacement &predicateReplacement)
	{
		if (!ast::matches(predicate, predicateReplacement.predicate))
			return;

		auto formulaReplacement = ast::prepareCopy(predicateReplacement.replacement);

		for (size_t i = 0; i < predicate.arguments.size(); i++)
		{
			assert(predicateReplacement.predicate.arguments[i].is<ast::Variable>());
			const auto &original = *predicateReplacement.predicate.arguments[i].get<ast::Variable>().declaration;

			assert(predicate.arguments[i].is<ast::Variable>());
			auto &replacement = *predicate.arguments[i].get<ast::Variable>().declaration;

			formulaReplacement.accept(ReplaceVariableInFormulaVisitor(), formulaReplacement, original, replacement);
		}

		formula = std::move(formulaReplacement);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &, const PredicateReplacement &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Detect whether a formula contains a circular dependency on a given predicate
struct DetectCircularDependcyVisitor : public ast::RecursiveFormulaVisitor<DetectCircularDependcyVisitor>
{
	static void accept(ast::Predicate &predicate, ast::Formula &, const ast::PredicateSignature &predicateSignature, bool &hasCircularDependency)
	{
		if (ast::matches(predicate, predicateSignature))
			hasCircularDependency = true;
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &, const ast::PredicateSignature &, bool &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Finds the replacement for predicates of the form “p(X1, ..., Xn) <-> q(X1, ..., Xn)”
PredicateReplacement findReplacement(const ast::PredicateSignature &predicateSignature, const ast::Predicate &predicate)
{
	// Declare variable used, only used in debug mode
	(void)(predicateSignature);

	assert(ast::matches(predicate, predicateSignature));

	// Replace with “#true”
	return {predicate, ast::Formula::make<ast::Boolean>(true)};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Finds the replacement for predicates of the form “p(X1, ..., Xn) <-> not q(X1, ..., Xn)”
PredicateReplacement findReplacement(const ast::PredicateSignature &predicateSignature, const ast::Not &not_)
{
	// Declare variable used, only used in debug mode
	(void)(predicateSignature);

	assert(not_.argument.is<ast::Predicate>());
	assert(ast::matches(not_.argument.get<ast::Predicate>(), predicateSignature));

	// Replace with “#false”
	return {not_.argument.get<ast::Predicate>(), ast::Formula::make<ast::Boolean>(false)};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Finds the replacement for predicates of the form “forall X1, ..., Xn (p(X1, ..., Xn) <-> ...)”
PredicateReplacement findReplacement(const ast::PredicateSignature &predicateSignature, const ast::Biconditional &biconditional)
{
	// Declare variable used, only used in debug mode
	(void)(predicateSignature);

	assert(biconditional.left.is<ast::Predicate>());
	assert(ast::matches(biconditional.left.get<ast::Predicate>(), predicateSignature));

	// TODO: avoid copy
	return {biconditional.left.get<ast::Predicate>(), ast::prepareCopy(biconditional.right)};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Finds a replacement for a predicate that should be hidden
PredicateReplacement findReplacement(const ast::PredicateSignature &predicateSignature, const ast::Formula &completedPredicateDefinition)
{
	// TODO: refactor
	if (completedPredicateDefinition.is<ast::ForAll>())
		return findReplacement(predicateSignature, completedPredicateDefinition.get<ast::ForAll>().argument);
	else if (completedPredicateDefinition.is<ast::Biconditional>())
		return findReplacement(predicateSignature, completedPredicateDefinition.get<ast::Biconditional>());
	else if (completedPredicateDefinition.is<ast::Predicate>())
		return findReplacement(predicateSignature, completedPredicateDefinition.get<ast::Predicate>());
	else if (completedPredicateDefinition.is<ast::Not>())
		return findReplacement(predicateSignature, completedPredicateDefinition.get<ast::Not>());

	throw CompletionException("unsupported completed definition for predicate “" + predicateSignature.name + "/" +  std::to_string(predicateSignature.arity) + "” for hiding predicates");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminateHiddenPredicates(const std::vector<ast::PredicateSignature> &predicateSignatures, std::vector<ast::Formula> &completedFormulas, Context &context)
{
	if (!context.visiblePredicateSignatures)
	{
		context.logger.log(output::Priority::Debug) << "no predicates to be eliminated";
		return;
	}

	auto &visiblePredicateSignatures = context.visiblePredicateSignatures.value();

	// Replace all occurrences of hidden predicates
	for (size_t i = 0; i < predicateSignatures.size(); i++)
	{
		auto &predicateSignature = predicateSignatures[i];

		const auto matchesPredicateSignature =
			[&](const auto &otherPredicateSignature)
			{
				return ast::matches(predicateSignature, otherPredicateSignature.predicateSignature);
			};

		const auto matchingVisiblePredicateSignature =
			std::find_if(visiblePredicateSignatures.begin(), visiblePredicateSignatures.end(), matchesPredicateSignature);

		// If the predicate ought to be visible, don’t eliminate it
		if (matchingVisiblePredicateSignature != visiblePredicateSignatures.end())
		{
			matchingVisiblePredicateSignature->used = true;
			continue;
		}

		// Check that the predicate is not declared #external
		if (context.externalPredicateSignatures)
		{
			const auto &externalPredicateSignatures = context.externalPredicateSignatures.value();

			const auto matchingExternalPredicateSignature =
				std::find_if(externalPredicateSignatures.cbegin(), externalPredicateSignatures.cend(), matchesPredicateSignature);

			if (matchingExternalPredicateSignature != externalPredicateSignatures.cend())
				continue;
		}

		context.logger.log(output::Priority::Debug) << "eliminating “" << predicateSignature.name << "/" << predicateSignature.arity << "”";

		const auto &completedPredicateDefinition = completedFormulas[i];
		auto replacement = findReplacement(predicateSignature, completedPredicateDefinition);

		bool hasCircularDependency = false;
		replacement.replacement.accept(DetectCircularDependcyVisitor(), replacement.replacement, predicateSignature, hasCircularDependency);

		if (hasCircularDependency)
		{
			context.logger.log(output::Priority::Warning) << "cannot hide predicate “" << predicateSignature.name << "/" << predicateSignature.arity << "” due to circular dependency";
			continue;
		}

		for (size_t j = 0; j < completedFormulas.size(); j++)
			if (j != i)
				completedFormulas[j].accept(ReplacePredicateInFormulaVisitor(), completedFormulas[j], replacement);

		// TODO: refactor
		completedFormulas[i] = ast::Formula::make<ast::Boolean>(true);
	}

	const auto canBeRemoved =
		[&](const ast::Formula &completedFormula)
		{
			if (!completedFormula.is<ast::Boolean>())
				return false;

			return completedFormula.get<ast::Boolean>().value == true;
		};

	auto removedFormulas = std::remove_if(completedFormulas.begin(), completedFormulas.end(), canBeRemoved);
	completedFormulas.erase(removedFormulas, completedFormulas.end());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
