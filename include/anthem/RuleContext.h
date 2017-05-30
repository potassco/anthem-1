#ifndef __ANTHEM__RULE_CONTEXT_H
#define __ANTHEM__RULE_CONTEXT_H

#include <clingo.hh>

#include <anthem/ASTForward.h>
#include <anthem/output/Logger.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RuleContext
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct RuleContext
{
	std::vector<const Clingo::AST::Term *> headTerms;
	ast::VariableDeclarationPointers freeVariables;

	// Index of the first auxiliary head variable into the vector freeVariables
	size_t headVariablesStartIndex = 0;

	bool isChoiceRule = false;
	size_t numberOfHeadLiterals = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
