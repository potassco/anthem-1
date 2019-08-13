/*#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/Context.h>
#include <anthem/StatementVisitor.h>
#include <anthem/examine-semantics/Translation.h>
#include <anthem/examine-semantics/TranslationPolicy.h>
#include <anthem/verify-properties/Translation.h>
#include <anthem/verify-properties/TranslationPolicy.h>
#include <anthem/verify-strong-equivalence/Translation.h>
#include <anthem/verify-strong-equivalence/TranslationPolicy.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////

template<class TranslationPolicy>
void translate(const std::vector<std::string> &fileNames, Context &context)
{
	if (fileNames.empty())
		throw TranslationException("no input files specified");

	typename TranslationPolicy::TranslationContext translationContext;



	switch (context.translationTarget)
	{
		case TranslationTarget::ExamineSemantics:
		{
			if (fileNames.size() > 1)
				throw TranslationException("only one file may me translated at a time when examining semantics");

			readSingleFile(fileNames.front());

			examineSemantics::translate(context, std::move(translationContext));
			break;
		}
		case TranslationTarget::VerifyProperties:
		{
			if (fileNames.size() > 1)
				throw TranslationException("only one file may me translated at a time when verifying properties");

			readSingleFile(fileNames.front());

			verifyProperties::translate(context, std::move(translationContext));
			break;
		}
		case TranslationTarget::VerifyStrongEquivalence:
		{
			if (fileNames.size() > 2)
				throw TranslationException("only one or two files may me translated at a time when verifying strong equivalence");

			auto scopedFormulasA = readSingleFile(fileNames.front());

			translationContext.

			auto scopedFormulasB = (fileNames.size() > 1)
				? std::optional<std::vector<ast::ScopedFormula>>(readSingleFile(fileNames[1]))
				: std::nullopt;

			verifyStrongEquivalence::translate(std::move(scopedFormulasA), std::move(scopedFormulasB), context);
			break;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class TranslationPolicy>
void translate(const char *fileName, std::istream &stream, Context &context)
{
	typename TranslationPolicy::TranslationContext translationContext;
	readSingleStream<TranslationPolicy>(fileName, stream, context, translationContext);
	TranslationPolicy::translate(context, std::move(translationContext));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}*/
