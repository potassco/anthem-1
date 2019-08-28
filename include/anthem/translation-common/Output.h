#ifndef __ANTHEM__TRANSLATION_COMMON__OUTPUT_H
#define __ANTHEM__TRANSLATION_COMMON__OUTPUT_H

#include <anthem/AST.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Output
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
				const char *ruleType = "";
				std::string formulaName;

				switch (formulaType)
				{
					case FormulaType::Axiom:
						ruleType = "axiom";
						formulaName = std::string(ruleType) + "_" + std::to_string(printContext.currentFormulaID + 1);
						break;
					case FormulaType::Conjecture:
						ruleType = "conjecture";
						formulaName = std::string(ruleType);
						break;
				}

				stream
					<< output::Keyword("tff")
					<< "(" << output::Function(formulaName.c_str())
					<< ", ";

				switch (formulaType)
				{
					case FormulaType::Axiom:
						stream << output::Keyword(ruleType);
						break;
					case FormulaType::Conjecture:
						stream << output::Keyword(ruleType);
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

template<class SymbolDeclaration>
struct PrintReturnTypeTrait
{
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<>
struct PrintReturnTypeTrait<ast::PredicateDeclaration>
{
	static output::ColorStream &print(output::ColorStream &stream,
		const ast::PredicateDeclaration &)
	{
		return (stream << output::Keyword("$o"));
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<>
struct PrintReturnTypeTrait<ast::FunctionDeclaration>
{
	static output::ColorStream &print(output::ColorStream &stream,
		const ast::FunctionDeclaration &functionDeclaration)
	{
		switch (functionDeclaration.domain)
		{
			case Domain::Integer:
				// TODO: clean up
				return (stream << output::Keyword("object"));
			case Domain::Symbolic:
				return (stream << output::Keyword("$i"));
			default:
				throw TranslationException("only functions with integer return type supported with TPTP currently");
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto printTypeAnnotation =
	[](const auto &symbolDeclaration, Context &context, output::PrintContext &printContext)
	{
		auto &stream = context.logger.outputStream();

		// TODO: clean up
		if (strcmp(symbolDeclaration.name.c_str(), AuxiliaryFunctionNameInteger) == 0
			|| strcmp(symbolDeclaration.name.c_str(), AuxiliaryFunctionNameSymbolic) == 0
			|| strcmp(symbolDeclaration.name.c_str(), AuxiliaryPredicateNameIsInteger) == 0)
		{
			return;
		}

		switch (context.outputFormat)
		{
			case OutputFormat::HumanReadable:
				for (auto i = 0; i < static_cast<int>(symbolDeclaration.parameters.size()); i++)
				{
					const auto &parameter = symbolDeclaration.parameters[i];

					if (parameter.domain != Domain::Integer)
						continue;

					stream
						<< output::Keyword("int")
						<< "(" << symbolDeclaration.name
						<< "/" << output::Number(symbolDeclaration.arity())
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
					<< ", (" << symbolDeclaration.name << ": ";

				if (!symbolDeclaration.parameters.empty())
					stream << "(";

				for (auto i = 0; i < static_cast<int>(symbolDeclaration.parameters.size()); i++)
				{
					if (i > 0)
						stream << " * ";

					// For TPTP, all program variable values are mapped to odd integer numbers, while integer
					// values n are mapped to 2 * n. This trick is necessary to translate variables that can take
					// values of both program and integer variables
					stream << output::Keyword("object");
				}

				if (!symbolDeclaration.parameters.empty())
					stream << ") > ";

				using PrintReturnTypeTrait = PrintReturnTypeTrait<typename std::remove_cv<
					typename std::remove_reference<decltype(symbolDeclaration)>::type>::type>;
				PrintReturnTypeTrait::print(stream, symbolDeclaration);
				stream << "))." << std::endl;

				break;
			}
		}

		printContext.currentTypeID++;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
