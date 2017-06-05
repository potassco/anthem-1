#ifndef __ANTHEM__CONTEXT_H
#define __ANTHEM__CONTEXT_H

#include <experimental/optional>

#include <anthem/AST.h>
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
	Context() = default;

	explicit Context(output::Logger &&logger)
	:	logger{std::move(logger)}
	{
	}

	output::Logger logger;

	bool performSimplification = false;
	bool performCompletion = false;

	std::experimental::optional<std::vector<ast::PredicateSignature>> visiblePredicateSignatures;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
