#ifndef __ANTHEM__VERIFY_PROPERTIES__TRANSLATION_CONTEXT_H
#define __ANTHEM__VERIFY_PROPERTIES__TRANSLATION_CONTEXT_H

#include <map>

#include <anthem/ASTForward.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TranslationContext
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct TranslationContext
{
	struct Definitions
	{
		ast::VariableDeclarationPointers headAtomParameters;
		std::vector<ast::ScopedFormula> definitions;
	};

	struct ConstantReplacementsComparator
	{
		bool operator()(const ast::FunctionDeclaration *x1, const ast::FunctionDeclaration *x2) const
		{
			return x1->name < x2->name;
		}
	};

	ast::VariableDeclarationPointers inputParameters;
	std::map<ast::FunctionDeclaration *, ast::VariableDeclaration *, ConstantReplacementsComparator> constantReplacements;

	std::map<ast::PredicateDeclaration *, Definitions> definitions;
	std::vector<ast::Formula> integrityConstraints;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
