#ifndef __ANTHEM__VERIFY_PROPERTIES__RULE_H
#define __ANTHEM__VERIFY_PROPERTIES__RULE_H

#include <anthem/AST.h>
#include <anthem/translation-common/Rule.h>
#include <anthem/verify-properties/Body.h>
#include <anthem/verify-properties/Head.h>
#include <anthem/verify-properties/TranslationContext.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Rule
//
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class FormulaType
{
	Definition,
	IntegrityConstraint,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline void read(const Clingo::AST::Rule &rule, Context &context, TranslationContext &translationContext)
{
	/*RuleContext ruleContext;
	ast::VariableStack variableStack;
	variableStack.push(&ruleContext.freeVariables);

	ast::And translatedBody;

	// Translate body literals
	for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
	{
		const auto &bodyLiteral = *i;

		auto argument = bodyLiteral.data.accept(BodyBodyLiteralVisitor(), bodyLiteral, context, ruleContext, variableStack);
		translatedBody.arguments.emplace_back(std::move(argument));
	}

	const auto headTranslationResult = rule.head.data.accept(HeadLiteralVisitor(), rule.head, context);

	const auto translateHeadTerms =
		[](const ast::Predicate &headAtom, ast::And &&formula)
		{
			for (const auto &argument : headAtom.arguments)
			{
				auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head);

				auto translatedHeadTerm = translationCommon::chooseValueInTerm(argument, variableDeclaration, context, ruleContext, variableStack);
				formula.emplace_back(translatedHeadTerm);

				ruleContext.freeVariables.emplace_back(variableDeclaration);
			}
		};

	switch (headTranslationResult.headType)
	{
		case HeadType::SingleAtom:
		{
			assert(headTranslationResult.predicate);
			const auto &headAtom = headTranslationResult.predicate.get();

			auto formula = std::move(translatedBody);
			translateHeadTerms(headAtom, formula);

			translationContext.definitions[headAtom->declaration].emplace_back(formula);
		}
		default
			// TODO: implement
			return;
	}

	throw LogicException("unreachable code, please report to bug tracker");*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
