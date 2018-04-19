#ifndef __ANTHEM__INTEGER_VARIABLE_DETECTION_H
#define __ANTHEM__INTEGER_VARIABLE_DETECTION_H

#include <anthem/AST.h>
#include <anthem/Context.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IntegerVariableDetection
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void detectIntegerVariables(std::vector<ast::Formula> &completedFormulas);

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif