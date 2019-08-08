#ifndef __ANTHEM__EXAMINE_SEMANTICS__SIMPLIFICATION_VISITORS_H
#define __ANTHEM__EXAMINE_SEMANTICS__SIMPLIFICATION_VISITORS_H

#include <anthem/AST.h>
#include <anthem/Utils.h>
#include <anthem/examine-semantics/Simplification.h>

namespace anthem
{
namespace examineSemantics
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification Visitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
struct FormulaSimplificationVisitor
{
	template <class... Arguments>
	OperationResult visit(ast::And &and_, ast::Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Biconditional &biconditional, ast::Formula &formula, Arguments &&... arguments)
	{
		if (biconditional.left.accept(*this, biconditional.left, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		if (biconditional.right.accept(*this, biconditional.right, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Boolean &, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Comparison &, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Exists &exists, ast::Formula &formula, Arguments &&... arguments)
	{
		if (exists.argument.accept(*this, exists.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::ForAll &forAll, ast::Formula &formula, Arguments &&... arguments)
	{
		if (forAll.argument.accept(*this, forAll.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Implies &implies, ast::Formula &formula, Arguments &&... arguments)
	{
		if (implies.antecedent.accept(*this, implies.antecedent, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		if (implies.consequent.accept(*this, implies.consequent, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::In &, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Not &not_, ast::Formula &formula, Arguments &&... arguments)
	{
		if (not_.argument.accept(*this, not_.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Or &or_, ast::Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ast::Predicate &, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
