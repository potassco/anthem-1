#ifndef __ANTHEM__AST_UTILS_H
#define __ANTHEM__AST_UTILS_H

#include <optional>

#include <anthem/AST.h>
#include <anthem/ASTVisitors.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST Utils
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: rename to VariableDeclarationStack or ParameterStack
// TODO: move to separate file
class VariableStack
{
	public:
		using Layer = VariableDeclarationPointers *;

	public:
		void push(Layer layer);
		void pop();

		std::optional<VariableDeclaration *> findUserVariableDeclaration(const char *variableName) const;
		bool contains(const VariableDeclaration &variableDeclaration) const;

	private:
		std::vector<Layer> m_layers;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matches(const Predicate &lhs, const Predicate &rhs);
bool matches(const Predicate &predicate, const PredicateSignature &signature);
bool matches(const PredicateSignature &lhs, const PredicateSignature &rhs);
void collectPredicateSignatures(const Formula &formula, std::vector<PredicateSignature> &predicateSignatures);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Replacing Variables
////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of a variable in a given term with another term
struct ReplaceVariableInTermVisitor : public RecursiveTermVisitor<ReplaceVariableInTermVisitor>
{
	static void accept(Variable &variable, Term &, const VariableDeclaration *original, VariableDeclaration *replacement)
	{
		if (variable.declaration == original)
			variable.declaration = replacement;
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, Term &, const VariableDeclaration *, VariableDeclaration *)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces all occurrences of a variable in a given formula with a term
struct ReplaceVariableInFormulaVisitor : public RecursiveFormulaVisitor<ReplaceVariableInFormulaVisitor>
{
	static void accept(Comparison &comparison, Formula &, const VariableDeclaration *original, VariableDeclaration *replacement)
	{
		comparison.left.accept(ReplaceVariableInTermVisitor(), comparison.left, original, replacement);
		comparison.right.accept(ReplaceVariableInTermVisitor(), comparison.right, original, replacement);
	}

	static void accept(In &in, Formula &, const VariableDeclaration *original, VariableDeclaration *replacement)
	{
		in.element.accept(ReplaceVariableInTermVisitor(), in.element, original, replacement);
		in.set.accept(ReplaceVariableInTermVisitor(), in.set, original, replacement);
	}

	static void accept(Predicate &predicate, Formula &, const VariableDeclaration *original, VariableDeclaration *replacement)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(ReplaceVariableInTermVisitor(), argument, original, replacement);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(T &, Formula &, const VariableDeclaration *, VariableDeclaration *)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
