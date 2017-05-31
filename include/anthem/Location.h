#ifndef __ANTHEM__INPUT__LOCATION_H
#define __ANTHEM__INPUT__LOCATION_H

#include <cstdlib>

#include <clingo.hh>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Location
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Location
{
	Location() = default;

	Location(const Clingo::Location &clingoLocation)
	:	sectionStart{clingoLocation.begin_file()},
		sectionEnd{clingoLocation.end_file()},
		rowStart{clingoLocation.begin_line()},
		rowEnd{clingoLocation.end_line()},
		columnStart{clingoLocation.begin_column()},
		columnEnd{clingoLocation.end_column()}
	{
	}

	Location(const Location &other) = default;
	Location &operator=(const Location &other) = default;
	Location(Location &&other) = default;
	Location &operator=(Location &&other) = default;

	const char *sectionStart = nullptr;
	const char *sectionEnd = nullptr;

	std::size_t rowStart = -1;
	std::size_t rowEnd = -1;

	std::size_t columnStart = -1;
	std::size_t columnEnd = -1;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
