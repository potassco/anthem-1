#ifndef __ANTHEM__SIMPLIFICATION_H
#define __ANTHEM__SIMPLIFICATION_H

#include <anthem/AST.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula simplify(ast::Formula &&formula);

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
