#ifndef __ANTHEM__SIMPLIFICATION_VISITORS_H
#define __ANTHEM__SIMPLIFICATION_VISITORS_H

#include <anthem/AST.h>
#include <anthem/Simplification.h>

namespace anthem
{
namespace ast
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
	SimplificationResult visit(And &and_, Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
				return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Biconditional &biconditional, Formula &formula, Arguments &&... arguments)
	{
		if (biconditional.left.accept(*this, biconditional.left, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		if (biconditional.right.accept(*this, biconditional.right, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Boolean &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Comparison &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Exists &exists, Formula &formula, Arguments &&... arguments)
	{
		if (exists.argument.accept(*this, exists.argument, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(ForAll &forAll, Formula &formula, Arguments &&... arguments)
	{
		if (forAll.argument.accept(*this, forAll.argument, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Implies &implies, Formula &formula, Arguments &&... arguments)
	{
		if (implies.antecedent.accept(*this, implies.antecedent, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		if (implies.consequent.accept(*this, implies.consequent, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(In &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Not &not_, Formula &formula, Arguments &&... arguments)
	{
		if (not_.argument.accept(*this, not_.argument, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Or &or_, Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
				return SimplificationResult::Simplified;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Predicate &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class SimplificationResult = void>
struct TermSimplificationVisitor
{
	template <class... Arguments>
	SimplificationResult visit(BinaryOperation &binaryOperation, Term &term, Arguments &&... arguments)
	{
		if (binaryOperation.left.accept(*this, binaryOperation.left, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		if (binaryOperation.right.accept(*this, binaryOperation.right, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Boolean &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Function &function, Term &term, Arguments &&... arguments)
	{
		for (auto &argument : function.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
				return SimplificationResult::Simplified;

		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Integer &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Interval &interval, Term &term, Arguments &&... arguments)
	{
		if (interval.from.accept(*this, interval.from, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		if (interval.to.accept(*this, interval.to, std::forward<Arguments>(arguments)...) == SimplificationResult::Simplified)
			return SimplificationResult::Simplified;

		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(SpecialInteger &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(String &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	SimplificationResult visit(Variable &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
