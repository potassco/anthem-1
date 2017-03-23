#ifndef __ANTHEM__AST_VISITORS_H
#define __ANTHEM__AST_VISITORS_H

#include <anthem/AST.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST Visitors
//
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
struct RecursiveFormulaVisitor
{
	template <class... Arguments>
	void visit(ast::And &and_, ast::Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(and_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Biconditional &biconditional, ast::Formula &formula, Arguments &&... arguments)
	{
		biconditional.left.accept(*this, biconditional.left, std::forward<Arguments>(arguments)...);
		biconditional.right.accept(*this, biconditional.right, std::forward<Arguments>(arguments)...);

		return T::accept(biconditional, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Boolean &boolean, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(boolean, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Comparison &comparison, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(comparison, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Exists &exists, ast::Formula &formula, Arguments &&... arguments)
	{
		exists.argument.accept(*this, exists.argument, std::forward<Arguments>(arguments)...);

		return T::accept(exists, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::ForAll &forAll, ast::Formula &formula, Arguments &&... arguments)
	{
		forAll.argument.accept(*this, forAll.argument, std::forward<Arguments>(arguments)...);

		return T::accept(forAll, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Implies &implies, ast::Formula &formula, Arguments &&... arguments)
	{
		implies.antecedent.accept(*this, implies.antecedent, std::forward<Arguments>(arguments)...);
		implies.consequent.accept(*this, implies.consequent, std::forward<Arguments>(arguments)...);

		return T::accept(implies, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::In &in, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(in, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Not &not_, ast::Formula &formula, Arguments &&... arguments)
	{
		not_.argument.accept(*this, not_.argument, std::forward<Arguments>(arguments)...);

		return T::accept(not_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Or &or_, ast::Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, argument, std::forward<Arguments>(arguments)...);

		return T::accept(or_, formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(ast::Predicate &predicate, ast::Formula &formula, Arguments &&... arguments)
	{
		return T::accept(predicate, formula, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
