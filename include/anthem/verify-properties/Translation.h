#ifndef __ANTHEM__VERIFY_PROPERTIES__TRANSLATION_H
#define __ANTHEM__VERIFY_PROPERTIES__TRANSLATION_H

#include <anthem/ASTForward.h>
#include <anthem/verify-properties/Rule.h>
#include <anthem/verify-properties/TranslationContext.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames, Context &context);
void translate(const char *fileName, std::istream &stream, Context &context);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
