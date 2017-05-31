#ifndef __ANTHEM__UTILS_H
#define __ANTHEM__UTILS_H

#include <iostream>

#include <clingo.hh>

#include <anthem/Context.h>
#include <anthem/Location.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Utils
//
////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isPrefix(const char *prefix, const char *string)
{
	const auto prefixLength = std::strlen(prefix);
	const auto stringLength = std::strlen(string);

	if (stringLength < prefixLength)
		return false;

	return std::strncmp(prefix, string, prefixLength) == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const auto HeadVariablePrefix = "V";
constexpr const auto BodyVariablePrefix = "X";
constexpr const auto UserVariablePrefix = "U";

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
