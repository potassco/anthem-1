#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/Completion.h>
#include <anthem/Context.h>
#include <anthem/IntegerVariableDetection.h>
#include <anthem/Simplification.h>
#include <anthem/StatementVisitor.h>
#include <anthem/output/FormatterHumanReadable.h>

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

		if (!file.is_open())
			throw LogicException("could not read file “" + fileName + "”");

		translate(fileName.c_str(), file, context);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream, Context &context)
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

	output::PrintContext printContext(context);

	if (context.headTranslationMode == HeadTranslationMode::Direct)
	{
		std::vector<ast::Formula> universallyClosedFormulas;
		universallyClosedFormulas.reserve(scopedFormulas.size());

		// Build the universal closure
		for (auto &scopedFormula : scopedFormulas)
		{
			auto universallyClosedFormula = (scopedFormula.freeVariables.empty())
				? std::move(scopedFormula.formula)
				: ast::Formula::make<ast::ForAll>(std::move(scopedFormula.freeVariables), std::move(scopedFormula.formula));

			universallyClosedFormulas.emplace_back(std::move(universallyClosedFormula));
		}

		// Detect integer variables
		if (context.performIntegerDetection)
			detectIntegerVariables(universallyClosedFormulas);

		// Simplify output if specified
		for (auto &universallyClosedFormula : universallyClosedFormulas)
		{
			if (context.performSimplification)
				simplify(universallyClosedFormula);

			output::print<output::FormatterHumanReadable>(context.logger.outputStream(), universallyClosedFormula, printContext);
			context.logger.outputStream() << std::endl;
		}

		return;
	}

	if (!context.performCompletion)
	{
		// Simplify output if specified
		if (context.performSimplification)
			for (auto &scopedFormula : scopedFormulas)
				simplify(scopedFormula.formula);

		if (context.showStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#show statements are ignored because completion is not enabled";

		if (context.externalStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#external statements are ignored because completion is not enabled";

		for (const auto &scopedFormula : scopedFormulas)
		{
			output::print<output::FormatterHumanReadable>(context.logger.outputStream(), scopedFormula.formula, printContext);
			context.logger.outputStream() << std::endl;
		}

		return;
	}

	// Perform completion
	auto completedFormulas = complete(std::move(scopedFormulas), context);

	for (const auto &predicateDeclaration : context.predicateDeclarations)
	{
		if (predicateDeclaration->isUsed)
			continue;

		// Check for #show statements with undeclared predicates
		if (predicateDeclaration->visibility != ast::PredicateDeclaration::Visibility::Default)
			context.logger.log(output::Priority::Warning)
				<< "#show declaration of “"
				<< predicateDeclaration->name
				<< "/"
				<< predicateDeclaration->arity()
				<< "” does not match any declared predicate";

		// Check for #external statements with undeclared predicates
		if (predicateDeclaration->isExternal && !predicateDeclaration->isUsed)
			context.logger.log(output::Priority::Warning)
				<< "#external declaration of “"
				<< predicateDeclaration->name
				<< "/"
				<< predicateDeclaration->arity()
				<< "” does not match any declared predicate";
	}

	// Detect integer variables
	if (context.performIntegerDetection)
		detectIntegerVariables(completedFormulas);

	// Simplify output if specified
	if (context.performSimplification)
		for (auto &completedFormula : completedFormulas)
			simplify(completedFormula);

	// TODO: remove variables that are not referenced after simplification

	for (const auto &completedFormula : completedFormulas)
	{
		output::print<output::FormatterHumanReadable>(context.logger.outputStream(), completedFormula, printContext);
		context.logger.outputStream() << std::endl;
	}

	// Print specifiers for integer predicate parameters
	for (auto &predicateDeclaration : context.predicateDeclarations)
	{
		// Check that the predicate is used and not declared #external
		if (!predicateDeclaration->isUsed || predicateDeclaration->isExternal)
			continue;

		const auto isPredicateVisible =
			(predicateDeclaration->visibility == ast::PredicateDeclaration::Visibility::Visible)
			|| (predicateDeclaration->visibility == ast::PredicateDeclaration::Visibility::Default
				&& context.defaultPredicateVisibility == ast::PredicateDeclaration::Visibility::Visible);

		// If the predicate ought to be visible, don’t eliminate it
		if (!isPredicateVisible)
			continue;

		for (size_t i = 0; i < predicateDeclaration->parameters.size(); i++)
		{
			auto &parameter = predicateDeclaration->parameters[i];

			if (parameter.domain != Domain::Integer)
				continue;

			context.logger.outputStream()
				<< output::Keyword("int")
				<< "(" << predicateDeclaration->name
				<< "/" << output::Number(predicateDeclaration->arity())
				<< "@" << output::Number(i + 1)
				<< ")" << std::endl;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
