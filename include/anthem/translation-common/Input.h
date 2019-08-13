#ifndef __ANTHEM__TRANSLATION_COMMON__INPUT_H
#define __ANTHEM__TRANSLATION_COMMON__INPUT_H

#include <fstream>
#include <string>

#include <anthem/Context.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Input
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TranslateStatementFunctor>
void readSingleStream(TranslateStatementFunctor &translateStatement, const char *fileName,
	std::istream &stream, Context &context)
{
	context.logger.log(output::Priority::Info) << "reading " << fileName;

	auto fileContent = std::string(std::istreambuf_iterator<char>(stream), {});

	const auto logger =
		[&context](const Clingo::WarningCode, const char *text)
		{
			context.logger.log(output::Priority::Error) << text;
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TranslateStatementFunctor>
void readSingleFile(TranslateStatementFunctor &translateStatement, const std::string &fileName,
	Context &context)
{
	std::ifstream file(fileName, std::ios::in);

	if (!file.is_open())
		throw LogicException("could not read file “" + fileName + "”");

	readSingleStream(translateStatement, fileName.c_str(), file, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
