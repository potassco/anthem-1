#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/Completion.h>
#include <anthem/Context.h>
#include <anthem/IntegerVariableDetection.h>
#include <anthem/MapDomains.h>
#include <anthem/Simplification.h>
#include <anthem/StatementVisitor.h>
#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class FormulaType
{
	Axiom,
	Conjecture,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto printFormula =
	[](const auto &value, FormulaType formulaType, Context &context, output::PrintContext &printContext)
	{
		auto &stream = context.logger.outputStream();

		switch (context.outputFormat)
		{
			case OutputFormat::HumanReadable:
				output::print<output::FormatterHumanReadable>(stream, value, printContext);
				break;
			case OutputFormat::TPTP:
			{
				const auto formulaName = std::string("axiom_") + std::to_string(printContext.currentFormulaID + 1);

				stream
					<< output::Keyword("tff")
					<< "(" << output::Function(formulaName.c_str())
					<< ", ";

				switch (formulaType)
				{
					case FormulaType::Axiom:
						stream << output::Keyword("axiom");
						break;
					case FormulaType::Conjecture:
						stream << output::Keyword("conjecture");
						break;
				}

				stream << ", ";
				output::print<output::FormatterTPTP>(stream, value, printContext);
				stream << ").";

				break;
			}
		}

		printContext.currentFormulaID++;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto printTypeAnnotation =
	[](const ast::PredicateDeclaration &predicateDeclaration, Context &context, output::PrintContext &printContext)
	{
		auto &stream = context.logger.outputStream();

		switch (context.outputFormat)
		{
			case OutputFormat::HumanReadable:
				for (size_t i = 0; i < predicateDeclaration.parameters.size(); i++)
				{
					const auto &parameter = predicateDeclaration.parameters[i];

					if (parameter.domain != Domain::Integer)
						continue;

					stream
						<< output::Keyword("int")
						<< "(" << predicateDeclaration.name
						<< "/" << output::Number(predicateDeclaration.arity())
						<< "@" << output::Number(i + 1)
						<< ")" << std::endl;
				}
				break;
			case OutputFormat::TPTP:
			{
				const auto typeName = std::string("type_") + std::to_string(printContext.currentTypeID + 1);

				stream
					<< output::Keyword("tff")
					<< "(" << output::Function(typeName.c_str())
					<< ", " << output::Keyword("type")
					<< ", (" << predicateDeclaration.name << ": (";

				for (size_t i = 0; i < predicateDeclaration.parameters.size(); i++)
				{
					if (i > 0)
						stream << " * ";

					// For TPTP, all program variable values are mapped to odd integer numbers, while integer
					// values n are mapped to 2 * n. This trick is necessary to translate variables that can take
					// values of both program and integer variables
					stream << output::Keyword("$int");
				}

				stream
					<< ") > " << output::Keyword("$o")
					<< "))." << std::endl;

				break;
			}
		}

		printContext.currentTypeID++;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

void translateCompletion(std::vector<ast::ScopedFormula> &&scopedFormulas, Context &context)
{
	assert(context.semantics == Semantics::ClassicalLogic);

	output::PrintContext printContext(context);

	const auto performSimplification = (context.performSimplification && context.semantics == Semantics::ClassicalLogic);

	if (!context.performCompletion)
	{
		// Simplify output if specified
		if (performSimplification)
			for (auto &scopedFormula : scopedFormulas)
				simplify(scopedFormula.formula);

		if (context.showStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#show statements are ignored because completion is not enabled";

		if (context.externalStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#external statements are ignored because completion is not enabled";

		for (const auto &scopedFormula : scopedFormulas)
		{
			printFormula(scopedFormula.formula, FormulaType::Axiom, context, printContext);
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
	if (performSimplification)
		for (auto &completedFormula : completedFormulas)
			simplify(completedFormula);

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

		printTypeAnnotation(*predicateDeclaration, context, printContext);
	}

	// TODO: remove variables that are not referenced after simplification

	for (const auto &completedFormula : completedFormulas)
	{
		printFormula(completedFormula, FormulaType::Axiom, context, printContext);
		context.logger.outputStream() << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translateHereAndThere(std::vector<ast::ScopedFormula> &&scopedFormulasA,
	std::optional<std::vector<ast::ScopedFormula>> &&scopedFormulasB, Context &context)
{
	output::PrintContext printContext(context);
	auto &stream = context.logger.outputStream();

	switch (context.semantics)
	{
		case Semantics::ClassicalLogic:
			context.logger.log(output::Priority::Info) << "output semantics: classical logic";
			break;
		case Semantics::LogicOfHereAndThere:
			context.logger.log(output::Priority::Warning) << "output semantics: logic of here-and-there";
			break;
	}

	const auto buildUniversallyClosedFormulas =
		[](std::vector<ast::ScopedFormula> &&scopedFormulas)
		{
			std::vector<ast::Formula> universallyClosedFormulas;
			universallyClosedFormulas.reserve(scopedFormulas.size());

			// Build the universal closure
			for (auto &scopedFormula : scopedFormulas)
			{
				const auto makeUniversallyClosedFormula =
					[&]() -> ast::Formula
					{
						if (scopedFormula.freeVariables.empty())
							return std::move(scopedFormula.formula);
					
						return ast::ForAll(std::move(scopedFormula.freeVariables), std::move(scopedFormula.formula));
					};

				universallyClosedFormulas.emplace_back(makeUniversallyClosedFormula());
			}

			return universallyClosedFormulas;
		};

	const auto buildFinalFormulas =
		[&]()
		{
			// If we’re just given one program, translate it to individual axioms
			if (!scopedFormulasB)
				return buildUniversallyClosedFormulas(std::move(scopedFormulasA));

			// If we’re given two programs A and B, translate them to a conjecture of the form “A <=> B”
			auto universallyClosedFormulasA = buildUniversallyClosedFormulas(std::move(scopedFormulasA));
			auto universallyClosedFormulasB = buildUniversallyClosedFormulas(std::move(scopedFormulasB.value()));

			// Build the conjunctions of all formulas resulting from each program respectively
			ast::And conjunctionA(std::move(universallyClosedFormulasA));
			ast::And conjunctionB(std::move(universallyClosedFormulasB));

			std::vector<ast::Formula> finalFormulas;
			finalFormulas.reserve(1);
			finalFormulas.emplace_back(ast::Biconditional(std::move(conjunctionA), std::move(conjunctionB)));

			return finalFormulas;
		};

	auto finalFormulas = buildFinalFormulas();

	// In case of TPTP output, map both program and integer variables to integers
	if (context.outputFormat == OutputFormat::TPTP)
		for (auto &finalFormula : finalFormulas)
			mapDomains(finalFormula, context);

	for (const auto &predicateDeclaration : context.predicateDeclarations)
		printTypeAnnotation(*predicateDeclaration, context, printContext);

	// Print auxiliary definitions for mapping program and integer variables to even and odd integers
	if (context.outputFormat == OutputFormat::TPTP)
		stream
			<< std::endl

			<< output::Keyword("tff")
			<< "(" << output::Function("is_even")
			<< ", " << output::Keyword("axiom")
			<< ", (" << output::Operator("!") << "["
			<< output::Variable("X") << ": " << output::Keyword("$int") << "]: ("
			<< output::Reserved(AuxiliaryPredicateNameEven) << "(" << output::Variable("X") << ") <=> ("
			<< output::Operator("?") << "["
			<< output::Variable("Y") << ": " << output::Keyword("$int") << "]: ("
			<< output::Variable("X") << " = " << output::Keyword("$product") << "("
			<< output::Number<int>(2) << ", " << output::Variable("Y") << "))"
			<< "))))." << std::endl

			<< output::Keyword("tff")
			<< "(" << output::Function("is_odd")
			<< ", " << output::Keyword("axiom")
			<< ", (" << output::Operator("!") << "["
			<< output::Variable("X") << ": " << output::Keyword("$int") << "]: ("
			<< output::Reserved(AuxiliaryPredicateNameOdd) << "(" << output::Variable("X") << ") <=> "
			<< output::Operator("~") << output::Reserved(AuxiliaryPredicateNameEven) << "(" << output::Variable("X")
			<< "))))." << std::endl;

	if (scopedFormulasB)
		assert(finalFormulas.size() == 1);

	const auto formulaType =
		[&scopedFormulasB]()
		{
			if (scopedFormulasB)
				return FormulaType::Conjecture;

			return FormulaType::Axiom;
		};

	// Print translated formulas
	for (auto &finalFormula : finalFormulas)
	{
		printFormula(finalFormula, formulaType(), context, printContext);
		context.logger.outputStream() << std::endl;
	}
}

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

	switch (context.translationMode)
	{
		case TranslationMode::Completion:
		{
			if (fileNames.size() > 1)
				throw TranslationException("only one file may me translated at a time in completion mode");

			auto scopedFormulas = translateSingleFile(fileNames.front());

			translateCompletion(std::move(scopedFormulas), context);
			break;
		}
		case TranslationMode::HereAndThere:
		{
			if (fileNames.size() > 2)
				throw TranslationException("only one or two files may me translated at a time in here-and-there mode");

			auto scopedFormulasA = translateSingleFile(fileNames.front());
			auto scopedFormulasB = (fileNames.size() > 1)
				? std::optional<std::vector<ast::ScopedFormula>>(translateSingleFile(fileNames[1]))
				: std::nullopt;

			translateHereAndThere(std::move(scopedFormulasA), std::move(scopedFormulasB), context);
			break;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream, Context &context)
{
	auto scopedFormulas = translateSingleStream(fileName, stream, context);

	switch (context.translationMode)
	{
		case TranslationMode::Completion:
		{
			translateCompletion(std::move(scopedFormulas), context);
			break;
		}
		case TranslationMode::HereAndThere:
		{
			translateHereAndThere(std::move(scopedFormulas), std::nullopt, context);
			break;
		}
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
