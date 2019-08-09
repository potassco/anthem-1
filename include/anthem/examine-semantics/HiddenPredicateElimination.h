#ifndef __ANTHEM__EXAMINE_SEMANTICS__HIDDEN_PREDICATE_ELIMINATION_H
#define __ANTHEM__EXAMINE_SEMANTICS__HIDDEN_PREDICATE_ELIMINATION_H

#include <anthem/AST.h>
#include <anthem/Context.h>

namespace anthem
{
namespace examineSemantics
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HiddenPredicateElimination
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminateHiddenPredicates(std::vector<ast::Formula> &completedFormulas, Context &context);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif