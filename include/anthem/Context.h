#ifndef __ANTHEM__CONTEXT_H
#define __ANTHEM__CONTEXT_H

#include <clingo.hh>

#include <anthem/output/Logger.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Context
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Context
{
	void reset()
	{
		headTerms.clear();
		isChoiceRule = false;
		numberOfHeadLiterals = 0;
		auxiliaryBodyVariableID = 1;
		anonymousVariableID = 1;
	}

	output::Logger logger;

	std::vector<const Clingo::AST::Term *> headTerms;
	bool isChoiceRule = false;
	size_t numberOfHeadLiterals = 0;
	size_t auxiliaryBodyVariableID = 1;
	size_t anonymousVariableID = 1;

	bool simplify = false;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
