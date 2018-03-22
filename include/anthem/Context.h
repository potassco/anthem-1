#ifndef __ANTHEM__CONTEXT_H
#define __ANTHEM__CONTEXT_H

#include <optional>

#include <anthem/AST.h>
#include <anthem/output/Logger.h>
#include <anthem/output/ParenthesisStyle.h>

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

	std::optional<std::vector<ast::PredicateSignature>> visiblePredicateSignatures;

	ast::ParenthesisStyle parenthesisStyle = ast::ParenthesisStyle::Normal;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
