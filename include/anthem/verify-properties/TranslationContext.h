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

	std::map<ast::PredicateDeclaration *, Definitions> definitions;
	std::vector<ast::Formula> integrityConstraints;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
