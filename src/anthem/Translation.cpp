#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

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

	bool isFirstStatement = true;

	const auto printFormula =
		[&context, &isFirstStatement](const ast::Formula &formula)
		{
			if (!isFirstStatement)
				context.logger.outputStream() << std::endl;

			context.logger.outputStream() << formula << std::endl;

			isFirstStatement = false;
		};

	const auto translateStatement =
		[&context, &printFormula](const Clingo::AST::Statement &statement)
		{
			auto formulas = statement.data.accept(StatementVisitor(), statement, context);

			for (auto &formula : formulas)
			{
				if (context.simplify)
					simplify(formula);

				printFormula(formula);
			}
		};

	const auto logger =
		[&context](const Clingo::WarningCode, const char *text)
		{
			context.logger.log(output::Priority::Error, text);
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
