#ifndef __ANTHEM__EXAMINE_SEMANTICS__RULE_H
#define __ANTHEM__EXAMINE_SEMANTICS__RULE_H

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/examine-semantics/Body.h>
#include <anthem/examine-semantics/Head.h>
#include <anthem/translation-common/Rule.h>

namespace anthem
{
namespace examineSemantics
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

	ast::And antecedent;
	std::optional<ast::Formula> consequent;

	// Collect all head terms
	rule.head.data.accept(examineSemantics::HeadLiteralCollectFunctionTermsVisitor(), rule.head, ruleContext);

	// Create new variable declarations for the head terms
	ruleContext.headVariablesStartIndex = ruleContext.freeVariables.size();
	ruleContext.freeVariables.reserve(ruleContext.headTerms.size());

	for (size_t i = 0; i < ruleContext.headTerms.size(); i++)
	{
		auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head);
		ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));
	}

	// Compute consequent
	auto headVariableIndex = ruleContext.headVariablesStartIndex;
	consequent = rule.head.data.accept(examineSemantics::HeadLiteralTranslateToConsequentVisitor(), rule.head, ruleContext, context, headVariableIndex);

	assert(ruleContext.headTerms.size() == headVariableIndex - ruleContext.headVariablesStartIndex);

	if (!consequent)
		throw TranslationException(rule.head.location, "could not translate formula consequent");

	// Generate auxiliary variables replacing the head atom’s arguments
	for (auto i = ruleContext.headTerms.cbegin(); i != ruleContext.headTerms.cend(); i++)
	{
		const auto &headTerm = **i;

		const auto auxiliaryHeadVariableID = ruleContext.headVariablesStartIndex + i - ruleContext.headTerms.cbegin();
		auto element = ast::Variable(ruleContext.freeVariables[auxiliaryHeadVariableID].get());
		auto set = translationCommon::translate(headTerm, ruleContext, context, variableStack);
		auto in = ast::In(std::move(element), std::move(set));

		antecedent.arguments.emplace_back(std::move(in));
	}

	// Translate body literals
	for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
	{
		const auto &bodyLiteral = *i;

		auto argument = bodyLiteral.data.accept(examineSemantics::BodyBodyLiteralTranslateVisitor(), bodyLiteral, ruleContext, context, variableStack);

		if (!argument)
			throw TranslationException(bodyLiteral.location, "could not translate body literal");

		antecedent.arguments.emplace_back(std::move(argument.value()));
	}

	if (!ruleContext.isChoiceRule)
	{
		ast::Implies formula(std::move(antecedent), std::move(consequent.value()));
		ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
		scopedFormulas.emplace_back(std::move(scopedFormula));
		translationCommon::normalizeAntecedent(scopedFormulas.back().formula.get<ast::Implies>());
	}
	else
	{
		const auto createFormula =
			[&](ast::Formula &argument, bool isLastOne)
			{
				auto &consequent = argument;

				if (!isLastOne)
				{
					ast::Implies formula(ast::prepareCopy(antecedent), std::move(consequent));
					ast::ScopedFormula scopedFormula(std::move(formula), {});
					ast::fixDanglingVariables(scopedFormula);
					scopedFormulas.emplace_back(std::move(scopedFormula));
				}
				else
				{
					ast::Implies formula(std::move(antecedent), std::move(consequent));
					ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
					scopedFormulas.emplace_back(std::move(scopedFormula));
				}

				auto &implies = scopedFormulas.back().formula.get<ast::Implies>();
				auto &antecedent = implies.antecedent.get<ast::And>();
				antecedent.arguments.emplace_back(ast::prepareCopy(implies.consequent));
				ast::fixDanglingVariables(scopedFormulas.back());

				translationCommon::normalizeAntecedent(implies);
			};

		if (consequent.value().is<ast::Or>())
		{
			auto &disjunction = consequent.value().get<ast::Or>();

			for (auto &argument : disjunction.arguments)
				createFormula(argument, &argument == &disjunction.arguments.back());
		}
		// TODO: check whether this is really correct for all possible consequent types
		else
			createFormula(consequent.value(), true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
