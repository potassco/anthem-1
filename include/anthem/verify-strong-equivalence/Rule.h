#ifndef __ANTHEM__VERIFY_STRONG_EQUIVALENCE__RULE_H
#define __ANTHEM__VERIFY_STRONG_EQUIVALENCE__RULE_H

#include <anthem/AST.h>
#include <anthem/translation-common/Rule.h>
#include <anthem/verify-strong-equivalence/Body.h>
#include <anthem/verify-strong-equivalence/Head.h>

namespace anthem
{
namespace verifyStrongEquivalence
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Rule
//
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void translate(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &, std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
{
	RuleContext ruleContext;
	ast::VariableStack variableStack;
	variableStack.push(&ruleContext.freeVariables);

	// Translate the head
	auto consequent = rule.head.data.accept(verifyStrongEquivalence::HeadLiteralTranslateToConsequentVisitor(), rule.head, context, ruleContext, variableStack);

	ast::And antecedent;

	// Translate body literals
	for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
	{
		const auto &bodyLiteral = *i;

		auto argument = bodyLiteral.data.accept(verifyStrongEquivalence::BodyBodyLiteralTranslateVisitor(), bodyLiteral, context, ruleContext, variableStack);
		antecedent.arguments.emplace_back(std::move(argument));
	}

	ast::Implies formula(std::move(antecedent), std::move(consequent));
	ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
	scopedFormulas.emplace_back(std::move(scopedFormula));
	translationCommon::normalizeAntecedent(scopedFormulas.back().formula.get<ast::Implies>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
