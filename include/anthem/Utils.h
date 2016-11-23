#ifndef __ANTHEM__UTILS_H
#define __ANTHEM__UTILS_H

#include <iostream>

#include <clingo.hh>

#include <anthem/Context.h>
#include <anthem/input/Location.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Utils
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T1, class T2>
T1 location_cast(const T2 &location);

////////////////////////////////////////////////////////////////////////////////////////////////////

template<>
input::Location location_cast(const Clingo::Location &location)
{
	return {location.begin_file(), location.end_file(), location.begin_line(), location.end_line(),
		location.begin_column(), location.end_column()};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline void throwErrorAtLocation(const Clingo::Location &clingoLocation, const char *errorMessage,
	Context &context)
{
	const auto location = location_cast<input::Location>(clingoLocation);

	context.logger.log(output::Priority::Error, location, errorMessage);

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const auto AuxiliaryHeadVariablePrefix = "AUX_H";
constexpr const auto AuxiliaryBodyVariablePrefix = "AUX_B";

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
