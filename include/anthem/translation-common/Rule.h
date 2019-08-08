#ifndef __ANTHEM__TRANSLATION_COMMON__RULE_H
#define __ANTHEM__TRANSLATION_COMMON__RULE_H

#include <anthem/AST.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Rule
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Replace empty and 1-element conjunctions in the antecedent of normal-form formulas
inline void normalizeAntecedent(ast::Implies &implies)
{
	if (!implies.antecedent.is<ast::And>())
		return;

	auto &antecedent = implies.antecedent.get<ast::And>();

	// Use “true” as the consequent in case it is empty
	if (antecedent.arguments.empty())
		implies.antecedent = ast::Boolean(true);
	else if (antecedent.arguments.size() == 1)
		implies.antecedent = std::move(antecedent.arguments[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
