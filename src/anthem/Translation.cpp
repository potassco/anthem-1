#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/Context.h>
#include <anthem/StatementVisitor.h>
#include <anthem/examine-semantics/Translation.h>
#include <anthem/verify-properties/Translation.h>
#include <anthem/verify-strong-equivalence/Translation.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::ScopedFormula> translateSingleStream(const char *fileName, std::istream &stream, Context &context)
{
	context.logger.log(output::Priority::Info) << "reading " << fileName;

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
			context.logger.log(output::Priority::Error) << text;
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);

	return scopedFormulas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames, Context &context)
{
	if (fileNames.empty())
		throw TranslationException("no input files specified");

	const auto translateSingleFile =
		[&](const auto &fileName)
		{
			std::ifstream file(fileName, std::ios::in);

			if (!file.is_open())
				throw LogicException("could not read file “" + fileName + "”");

			return translateSingleStream(fileName.c_str(), file, context);
		};

	switch (context.translationTarget)
	{
		case TranslationTarget::ExamineSemantics:
		{
			if (fileNames.size() > 1)
				throw TranslationException("only one file may me translated at a time when examining semantics");

			auto scopedFormulas = translateSingleFile(fileNames.front());

			examineSemantics::translate(std::move(scopedFormulas), context);
			break;
		}
		case TranslationTarget::VerifyProperties:
		{
			if (fileNames.size() > 1)
				throw TranslationException("only one file may me translated at a time when verifying properties");

			auto scopedFormulas = translateSingleFile(fileNames.front());

			verifyProperties::translate(std::move(scopedFormulas), context);
			break;
		}
		case TranslationTarget::VerifyStrongEquivalence:
		{
			if (fileNames.size() > 2)
				throw TranslationException("only one or two files may me translated at a time when verifying strong equivalence");

			auto scopedFormulasA = translateSingleFile(fileNames.front());
			auto scopedFormulasB = (fileNames.size() > 1)
				? std::optional<std::vector<ast::ScopedFormula>>(translateSingleFile(fileNames[1]))
				: std::nullopt;

			verifyStrongEquivalence::translate(std::move(scopedFormulasA), std::move(scopedFormulasB), context);
			break;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream, Context &context)
{
	auto scopedFormulas = translateSingleStream(fileName, stream, context);

	switch (context.translationTarget)
	{
		case TranslationTarget::ExamineSemantics:
		{
			examineSemantics::translate(std::move(scopedFormulas), context);
			break;
		}
		case TranslationTarget::VerifyProperties:
		{
			verifyProperties::translate(std::move(scopedFormulas), context);
			break;
		}
		case TranslationTarget::VerifyStrongEquivalence:
		{
			verifyStrongEquivalence::translate(std::move(scopedFormulas), std::nullopt, context);
			break;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
