#ifndef __ANTHEM__AST_VISITORS_H
#define __ANTHEM__AST_VISITORS_H

#include <anthem/AST.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST Visitors
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: refactor
template<class T, class ReturnType = void>
struct RecursiveFormulaVisitor
{
	template <class... Arguments>
	ReturnType visit(And &and_, Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(and_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Biconditional &biconditional, Formula &formula, Arguments &&... arguments)
	{
		biconditional.left.accept(*this, biconditional.left, std::forward<Arguments>(arguments)...);
		biconditional.right.accept(*this, biconditional.right, std::forward<Arguments>(arguments)...);

		return T::accept(biconditional, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Boolean &boolean, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(boolean, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Comparison &comparison, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(comparison, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Exists &exists, Formula &formula, Arguments &&... arguments)
	{
		exists.argument.accept(*this, exists.argument, std::forward<Arguments>(arguments)...);

		return T::accept(exists, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(ForAll &forAll, Formula &formula, Arguments &&... arguments)
	{
		forAll.argument.accept(*this, forAll.argument, std::forward<Arguments>(arguments)...);

		return T::accept(forAll, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Implies &implies, Formula &formula, Arguments &&... arguments)
	{
		implies.antecedent.accept(*this, implies.antecedent, std::forward<Arguments>(arguments)...);
		implies.consequent.accept(*this, implies.consequent, std::forward<Arguments>(arguments)...);

		return T::accept(implies, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(In &in, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(in, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Not &not_, Formula &formula, Arguments &&... arguments)
	{
		not_.argument.accept(*this, not_.argument, std::forward<Arguments>(arguments)...);

		return T::accept(not_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Or &or_, Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(or_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Predicate &predicate, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(predicate, formula, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class ReturnType = void>
struct RecursiveTermVisitor
{
	template <class... Arguments>
	ReturnType visit(BinaryOperation &binaryOperation, Term &term, Arguments &&... arguments)
	{
		binaryOperation.left.accept(*this, binaryOperation.left, std::forward<Arguments>(arguments)...);
		binaryOperation.right.accept(*this, binaryOperation.right, std::forward<Arguments>(arguments)...);

		return T::accept(binaryOperation, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Boolean &boolean, Term &term, Arguments &&... arguments)
	{
		return T::accept(boolean, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Function &function, Term &term, Arguments &&... arguments)
	{
		for (auto &argument : function.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(function, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Integer &integer, Term &term, Arguments &&... arguments)
	{
		return T::accept(integer, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Interval &interval, Term &term, Arguments &&... arguments)
	{
		interval.from.accept(*this, interval.from, std::forward<Arguments>(arguments)...);
		interval.to.accept(*this, interval.to, std::forward<Arguments>(arguments)...);

		return T::accept(interval, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(SpecialInteger &specialInteger, Term &term, Arguments &&... arguments)
	{
		return T::accept(specialInteger, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(String &string, Term &term, Arguments &&... arguments)
	{
		return T::accept(string, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(UnaryOperation &unaryOperation, Term &term, Arguments &&... arguments)
	{
		unaryOperation.argument.accept(*this, unaryOperation.argument, std::forward<Arguments>(arguments)...);

		return T::accept(unaryOperation, term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	ReturnType visit(Variable &variable, Term &term, Arguments &&... arguments)
	{
		return T::accept(variable, term, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
