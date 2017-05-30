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

	std::vector<ast::ScopedFormula> scopedFormulas;

	const auto translateStatement =
		[&scopedFormulas, &context](const Clingo::AST::Statement &statement)
		{
			statement.data.accept(StatementVisitor(), statement, scopedFormulas, context);
		};

	const auto logger =
		[&context](const Clingo::WarningCode, const char *text)
		{
			context.logger.log(output::Priority::Error, text);
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);

	if (context.simplify)
		for (auto &scopedFormula : scopedFormulas)
			simplify(scopedFormula.formula);

	if (context.complete)
		complete(scopedFormulas);

	ast::PrintContext printContext;

	for (const auto &scopedFormula : scopedFormulas)
	{
		ast::print(context.logger.outputStream(), scopedFormula.formula, printContext);
		context.logger.outputStream() << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
