#ifndef __ANTHEM__VERIFY_PROPERTIES__HEAD_H
#define __ANTHEM__VERIFY_PROPERTIES__HEAD_H

#include <algorithm>
#include <optional>

#include <anthem/AST.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/translation-common/ChooseValueInTerm.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Head
//
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class HeadType
{
	SingleAtom,
	ChoiceSingleAtom,
	IntegrityConstraint,
	Fact,
	Annotation,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadAtom
{
	ast::PredicateDeclaration *predicateDeclaration;
	const std::vector<Clingo::AST::Term> &arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadTranslationResult
{
	HeadType headType;
	std::optional<HeadAtom> headAtom;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline HeadAtom makeHeadAtom(const Clingo::AST::Function &function, Context &context)
{
	auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
	predicateDeclaration->isUsed = true;

	return HeadAtom{predicateDeclaration, function.arguments};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralVisitor
{
	HeadTranslationResult visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
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

		return HeadTranslationResult{HeadType::ChoiceSingleAtom, makeHeadAtom(function, context)};
	}

	HeadTranslationResult visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated head literals not yet supported in rule head");

		if (literal.data.is<Clingo::AST::Boolean>())
		{
			if (literal.data.get<Clingo::AST::Boolean>().value == true)
				return HeadTranslationResult{HeadType::Fact, std::nullopt};

			return HeadTranslationResult{HeadType::IntegrityConstraint, std::nullopt};
		}

		if (!literal.data.is<Clingo::AST::Term>())
			throw TranslationException(headLiteral.location, "only terms currently supported in literals in rule head");

		const auto &term = literal.data.get<Clingo::AST::Term>();

		if (!term.data.is<Clingo::AST::Function>())
			throw TranslationException(headLiteral.location, "only atoms currently supported in literals in rule head");

		const auto &function = term.data.get<Clingo::AST::Function>();

		return HeadTranslationResult{HeadType::SingleAtom, makeHeadAtom(function, context)};
	}

	HeadTranslationResult visit(const Clingo::AST::TheoryAtom &theoryAtom, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		if (theoryAtom.guard)
			throw TranslationException(headLiteral.location, "theory guards not allowed in annotations");

		if (!theoryAtom.elements.empty())
			throw TranslationException(headLiteral.location, "theory atom elements not allowed in annotations");

		if (!theoryAtom.term.data.is<Clingo::AST::Function>())
			throw TranslationException(theoryAtom.term.location, "annotations must use theory atoms containing a function");

		const auto &function = theoryAtom.term.data.get<Clingo::AST::Function>();

		if (strcmp(function.name, "input") != 0)
			throw TranslationException(theoryAtom.term.location, "only input annotations allowed currently");

		if (function.arguments.empty())
			throw TranslationException(theoryAtom.term.location, "missing argument for input annotation");

		const auto &typeArgument = function.arguments[0];

		if (!typeArgument.data.is<Clingo::AST::Function>())
			throw TranslationException(typeArgument.location, "expected type identifier");

		const auto &typeEnclosure = typeArgument.data.get<Clingo::AST::Function>();

		if (typeEnclosure.arguments.size() != 1)
			throw TranslationException(typeArgument.location, "expected type identifier");

		const auto &typeSpecifier = typeEnclosure.arguments[0];

		if (strcmp(typeEnclosure.name, "constant") == 0)
		{
			if (function.arguments.size() != 2)
				throw TranslationException(theoryAtom.term.location, "2 arguments expected for input constant annotation");

			if (!typeSpecifier.data.is<Clingo::Symbol>())
				throw TranslationException(typeArgument.location, "invalid type identifier");

			const auto &typeSpecifierSymbol = typeSpecifier.data.get<Clingo::Symbol>();

			if (typeSpecifierSymbol.type() != Clingo::SymbolType::Function)
				throw TranslationException(typeArgument.location, "invalid type identifier");

			const auto &typeSpecifierName = typeSpecifierSymbol.name();

			const auto readSort =
				[&]()
				{
					const auto &sortArgument = function.arguments[1];

					if (!sortArgument.data.is<Clingo::AST::Function>())
						throw TranslationException(sortArgument.location, "expected sort identifier");

					const auto &sortEnclosure = sortArgument.data.get<Clingo::AST::Function>();

					if (strcmp(sortEnclosure.name, "sort") != 0)
						throw TranslationException(sortArgument.location, "expected sort identifier");

					if (sortEnclosure.arguments.size() != 1)
						throw TranslationException(sortArgument.location, "expected only one sort identifier");

					const auto &sortSpecifier = sortEnclosure.arguments[0];

					if (!sortSpecifier.data.is<Clingo::Symbol>())
						throw TranslationException(sortSpecifier.location, "invalid sort identifier");

					const auto &sortSpecifierSymbol = sortSpecifier.data.get<Clingo::Symbol>();

					if (sortSpecifierSymbol.type() != Clingo::SymbolType::Function)
						throw TranslationException(sortSpecifier.location, "invalid sort identifier");

					if (strcmp(sortSpecifierSymbol.name(), "program") == 0)
						return Domain::Program;
					else if (strcmp(sortSpecifierSymbol.name(), "integer") == 0)
						return Domain::Integer;

					throw TranslationException(sortSpecifier.location, "unknown sort identifier");
				};

			const auto sort = readSort();

			std::cout << "found input constant declaration for " << typeSpecifierName;

			if (sort == Domain::Integer)
				std::cout << " of type integer" << std::endl;
			else
				std::cout << " of type program" << std::endl;
		}
		else if (strcmp(typeEnclosure.name, "predicate") == 0)
		{
			if (function.arguments.size() != 1)
				throw TranslationException(theoryAtom.term.location, "1 argument expected for input predicate annotation");

			if (!typeSpecifier.data.is<Clingo::AST::BinaryOperation>())
				throw TranslationException(typeSpecifier.location, "invalid predicate identifier");

			const auto &typeSpecifierBinaryOperation = typeSpecifier.data.get<Clingo::AST::BinaryOperation>();

			if (typeSpecifierBinaryOperation.binary_operator != Clingo::AST::BinaryOperator::Division)
				throw TranslationException(typeSpecifier.location, "invalid predicate identifier");

			const auto &predicateNameTerm = typeSpecifierBinaryOperation.left;

			if (!predicateNameTerm.data.is<Clingo::Symbol>())
				throw TranslationException(predicateNameTerm.location, "invalid predicate identifier");

			const auto &predicateNameSymbol = predicateNameTerm.data.get<Clingo::Symbol>();

			if (predicateNameSymbol.type() != Clingo::SymbolType::Function)
				throw TranslationException(predicateNameTerm.location, "invalid predicate identifier");

			const auto &predicateName = predicateNameSymbol.name();

			const auto &predicateArityTerm = typeSpecifierBinaryOperation.right;

			if (!predicateArityTerm.data.is<Clingo::Symbol>())
				throw TranslationException(predicateArityTerm.location, "invalid predicate arity");

			const auto &predicateAritySymbol = predicateArityTerm.data.get<Clingo::Symbol>();

			if (predicateAritySymbol.type() != Clingo::SymbolType::Number)
				throw TranslationException(predicateArityTerm.location, "invalid predicate arity");

			const auto predicateArity = predicateAritySymbol.number();

			if (predicateArity < 0)
				throw TranslationException(predicateArityTerm.location, "invalid predicate arity");

			const auto predicateDeclaration = context.findOrCreatePredicateDeclaration(predicateName, predicateArity);

			context.inputPredicateDeclarations.emplace(predicateDeclaration);
		}
		else
			throw TranslationException(typeArgument.location, "“input” annotations only supported for constants currently");

		return HeadTranslationResult{HeadType::Annotation, std::nullopt};
	}

	template<class T>
	HeadTranslationResult visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, Context &)
	{
		throw TranslationException(headLiteral.location, "head literal not yet supported in rule head, expected literal or aggregate");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
