#ifndef __ANTHEM__HEAD_DIRECT_H
#define __ANTHEM__HEAD_DIRECT_H

#include <algorithm>
#include <optional>

#include <anthem/AST.h>
#include <anthem/ChooseValueInTerm.h>
#include <anthem/ComparisonOperator.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace direct
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// HeadDirect
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula makeHeadFormula(const Clingo::AST::Function &function, bool isChoiceRule, Context &context, RuleContext &ruleContext, ast::VariableStack &variableStack)
{
	auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
	predicateDeclaration->isUsed = true;

	ast::VariableDeclarationPointers parameters;
	parameters.reserve(function.arguments.size());

	for (int i = 0; i < static_cast<int>(function.arguments.size()); i++)
	{
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
		// TODO: should be Domain::Unknown
		parameters.back()->domain = Domain::Symbolic;
	}

	ast::And and_;
	and_.arguments.reserve(parameters.size());

	for (int i = 0; i < static_cast<int>(function.arguments.size()); i++)
	{
		auto &argument = function.arguments[i];
		and_.arguments.emplace_back(chooseValueInTerm(argument, *parameters[i], context, ruleContext, variableStack));
	}

	const auto makePredicate =
		[&]()
		{
			ast::Predicate predicate(predicateDeclaration);

			for (int i = 0; i < static_cast<int>(function.arguments.size()); i++)
				predicate.arguments.emplace_back(ast::Variable(parameters[i].get()));

			return predicate;
		};

	const auto makeImplication =
		[&]() -> ast::Formula
		{
			if (!isChoiceRule)
				return makePredicate();

			ast::Or or_;
			or_.arguments.reserve(2);
			or_.arguments.emplace_back(makePredicate());
			or_.arguments.emplace_back(ast::Not(makePredicate()));

			return std::move(or_);
		};

	ast::Implies implies(std::move(and_), makeImplication());

	return ast::ForAll(std::move(parameters), std::move(implies));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralTranslateToConsequentVisitor
{
	ast::Formula visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		if (aggregate.left_guard || aggregate.right_guard)
			throw TranslationException(headLiteral.location, "aggregates with left or right guards not yet supported in rule head");

		if (aggregate.elements.size() != 1)
			throw TranslationException("aggregates with more than one element not yet supported in rule head");

		if (!aggregate.elements[0].condition.empty())
			throw TranslationException(headLiteral.location, "conditional literals not yet supported in rule head");

		if (aggregate.elements[0].literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(headLiteral.location, "negated literals in aggregates not yet supported in rule head");

		const auto &literal = aggregate.elements[0].literal;

		if (!literal.data.is<Clingo::AST::Term>())
			throw TranslationException(headLiteral.location, "only terms currently supported in aggregates in rule head");

		const auto &term = literal.data.get<Clingo::AST::Term>();

		if (!term.data.is<Clingo::AST::Function>())
			throw TranslationException(headLiteral.location, "only atoms currently supported in aggregates in rule head");

		const auto &function = term.data.get<Clingo::AST::Function>();

		// Choice rules require us to translate the rules to formulas in the logic of here-and-there
		context.semantics = Semantics::LogicOfHereAndThere;

		return makeHeadFormula(function, true, context, ruleContext, variableStack);
	}

	ast::Formula visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated head literals not yet supported in rule head");

		if (literal.data.is<Clingo::AST::Boolean>())
			return ast::Boolean(literal.data.get<Clingo::AST::Boolean>().value);

		if (!literal.data.is<Clingo::AST::Term>())
			throw TranslationException(headLiteral.location, "only terms currently supported in literals in rule head");

		const auto &term = literal.data.get<Clingo::AST::Term>();

		if (!term.data.is<Clingo::AST::Function>())
			throw TranslationException(headLiteral.location, "only atoms currently supported in literals in rule head");

		const auto &function = term.data.get<Clingo::AST::Function>();

		return makeHeadFormula(function, false, context, ruleContext, variableStack);
	}

	ast::Formula visit(const Clingo::AST::TheoryAtom &theoryAtom, const Clingo::AST::HeadLiteral &headLiteral, Context &context, RuleContext &, ast::VariableStack &)
	{
		const auto throwInvalidFormat =
			[&headLiteral]()
			{
				throw TranslationException(headLiteral.location, "invalid type declaration format");
			};

		if (theoryAtom.guard || !theoryAtom.term.data.is<Clingo::AST::Function>())
			throwInvalidFormat();

		auto &function = theoryAtom.term.data.get<Clingo::AST::Function>();

		if (strcmp(function.name, "type") != 0 || !function.arguments.empty())
			throwInvalidFormat();

		for (const auto &element : theoryAtom.elements)
		{
			if (!element.condition.empty() || element.tuple.size() != 1)
				throwInvalidFormat();

			const auto &term = element.tuple.front();

			if (!term.data.is<Clingo::AST::TheoryUnparsedTerm>())
				throwInvalidFormat();

			const auto &unparsedTerm = term.data.get<Clingo::AST::TheoryUnparsedTerm>();

			if (unparsedTerm.elements.size() != 1)
				throwInvalidFormat();

			const auto &unparsedTermElement = unparsedTerm.elements.front();

			if (!unparsedTermElement.operators.empty() || !unparsedTermElement.term.data.is<Clingo::AST::TheoryFunction>())
				throwInvalidFormat();

			const auto &theoryFunction = unparsedTermElement.term.data.get<Clingo::AST::TheoryFunction>();

			const auto name = theoryFunction.name;
			const auto arity = theoryFunction.arguments.size();

			auto predicateDeclaration = context.findOrCreatePredicateDeclaration(name, arity);

			for (int i = 0; i < static_cast<int>(theoryFunction.arguments.size()); i++)
			{
				const auto &argument = theoryFunction.arguments[i];

				if (!argument.data.is<Clingo::AST::TheoryUnparsedTerm>())
					throwInvalidFormat();

				const auto &unparsedTerm = argument.data.get<Clingo::AST::TheoryUnparsedTerm>();

				if (unparsedTerm.elements.size() != 1)
					throwInvalidFormat();

				const auto &unparsedTermElement = unparsedTerm.elements.front();

				if (!unparsedTermElement.operators.empty() || !unparsedTermElement.term.data.is<Clingo::Symbol>())
					throwInvalidFormat();

				const auto &symbol = unparsedTermElement.term.data.get<Clingo::Symbol>();

				if (symbol.type() != Clingo::SymbolType::Function || !symbol.arguments().empty())
					throwInvalidFormat();

				const auto type = symbol.name();

				if (strcmp(type, "integer") == 0)
					predicateDeclaration->parameters[i].domain = Domain::Integer;
				else if (strcmp(type, "symbolic") == 0)
					predicateDeclaration->parameters[i].domain = Domain::Symbolic;
				else
					throwInvalidFormat();

				context.typeStatementUsed = true;
			}
		}

		return ast::Boolean(true);
	}

	template<class T>
	ast::Formula visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, Context &, RuleContext &, ast::VariableStack &)
	{
		throw TranslationException(headLiteral.location, "head literal not yet supported in rule head, expected literal or aggregate");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
