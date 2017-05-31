#ifndef __ANTHEM__AST_UTILS_H
#define __ANTHEM__AST_UTILS_H

#include <experimental/optional>

#include <anthem/AST.h>

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

		std::experimental::optional<VariableDeclaration *> findUserVariableDeclaration(const char *variableName) const;
		bool contains(const VariableDeclaration &variableDeclaration) const;

	private:
		std::vector<Layer> m_layers;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<VariableDeclaration *> collectFreeVariables(Formula &formula);
std::vector<VariableDeclaration *> collectFreeVariables(Formula &formula, VariableStack &variableStack);

bool matches(const Predicate &lhs, const Predicate &rhs);
void collectPredicates(const Formula &formula, std::vector<const Predicate *> &predicates);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
