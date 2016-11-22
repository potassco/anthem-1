#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/StatementVisitor.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames)
{
	for (const auto &fileName : fileNames)
	{
		std::ifstream file(fileName, std::ios::in);

		translate(fileName.c_str(), file);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream)
{
	std::cout << "info: reading " << fileName << std::endl;

	auto fileContent = std::string(std::istreambuf_iterator<char>(stream), {});

	const auto translateStatement =
		[](const Clingo::AST::Statement &statement)
		{
			statement.data.accept(StatementVisitor(), statement);
			std::cout << std::endl;
		};

	const auto logger =
		[](const auto warningCode, const auto *text)
		{
			std::cout << "warning: " << text << std::endl;
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
