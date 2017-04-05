#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/Completion.h>
#include <anthem/Context.h>
#include <anthem/Simplification.h>
#include <anthem/StatementVisitor.h>
#include <anthem/output/AST.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames, Context &context)
{
	for (const auto &fileName : fileNames)
	{
		std::ifstream file(fileName, std::ios::in);

		translate(fileName.c_str(), file, context);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream, Context &context)
{
	// TODO: refactor
	context.logger.log(output::Priority::Info, (std::string("reading ") + fileName).c_str());

	auto fileContent = std::string(std::istreambuf_iterator<char>(stream), {});

	std::vector<ast::Formula> formulas;

	const auto translateStatement =
		[&formulas, &context](const Clingo::AST::Statement &statement)
		{
			statement.data.accept(StatementVisitor(), statement, formulas, context);
		};

	const auto logger =
		[&context](const Clingo::WarningCode, const char *text)
		{
			context.logger.log(output::Priority::Error, text);
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);

	if (context.complete)
		complete(formulas);

	for (auto i = formulas.begin(); i != formulas.end(); i++)
	{
		auto &formula = *i;

		if (context.simplify)
			simplify(formula);

		context.logger.outputStream() << formula << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
