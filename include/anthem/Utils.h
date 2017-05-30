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
inline input::Location location_cast(const Clingo::Location &location)
{
	return {location.begin_file(), location.end_file(), location.begin_line(), location.end_line(),
		location.begin_column(), location.end_column()};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: refactor
inline void throwErrorAtLocation(const Clingo::Location &clingoLocation, const char *errorMessage,
	Context &context)
{
	const auto location = location_cast<input::Location>(clingoLocation);

	// TODO: think about removing this to avoid double error messages
	context.logger.log(output::Priority::Error, location) << errorMessage;

	throw std::runtime_error(errorMessage);
}

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
