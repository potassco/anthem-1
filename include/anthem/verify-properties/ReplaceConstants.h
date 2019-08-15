#ifndef __ANTHEM__VERIFY_PROPERTIES__REPLACE_CONSTANTS_H
#define __ANTHEM__VERIFY_PROPERTIES__REPLACE_CONSTANTS_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ASTVisitors.h>
#include <anthem/verify-properties/TranslationContext.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void replaceConstants(ast::Formula &formula, TranslationContext &translationContext);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ReplaceConstantsInTermVisitor
{
	void visit(ast::Function &function, ast::Term &term, TranslationContext &translationContext)
	{
		if (function.declaration->arity() > 0)
			return;

		auto matchingConstantReplacement = translationContext.constantReplacements.find(
			function.declaration);

		if (matchingConstantReplacement == translationContext.constantReplacements.cend())
		{
			auto inputParameter = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Input);

			translationContext.inputParameters.emplace_back(std::move(inputParameter));

			translationContext.constantReplacements.insert(std::make_pair(
				function.declaration, translationContext.inputParameters.back().get()));

			matchingConstantReplacement = translationContext.constantReplacements.find(
				function.declaration);
		}

		term = ast::Variable(matchingConstantReplacement->second);
	}

	template<class T>
	void visit(T &, ast::Term &, TranslationContext &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of constants in a given formula with variables
struct ReplaceConstantsInFormulaVisitor : public ast::RecursiveFormulaVisitor<ReplaceConstantsInFormulaVisitor>
{
	static void accept(ast::Comparison &comparison, ast::Formula &,
		TranslationContext &translationContext)
	{
		comparison.left.accept(ReplaceConstantsInTermVisitor(), comparison.left, translationContext);
		comparison.right.accept(ReplaceConstantsInTermVisitor(), comparison.right, translationContext);
	}

	static void accept(ast::In &in, ast::Formula &, TranslationContext &translationContext)
	{
		in.element.accept(ReplaceConstantsInTermVisitor(), in.element, translationContext);
		in.set.accept(ReplaceConstantsInTermVisitor(), in.set, translationContext);
	}

	static void accept(ast::Predicate &predicate, ast::Formula &, TranslationContext &translationContext)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(ReplaceConstantsInTermVisitor(), argument, translationContext);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, ast::Formula &, TranslationContext &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline void replaceConstants(ast::Formula &formula, TranslationContext &translationContext)
{
	formula.accept(ReplaceConstantsInFormulaVisitor(), formula, translationContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
