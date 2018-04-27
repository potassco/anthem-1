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

constexpr const auto HeadVariablePrefix = "V";
constexpr const auto BodyVariablePrefix = "X";
constexpr const auto UserVariablePrefix = "U";

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Tristate
{
	True,
	False,
	Unknown,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
