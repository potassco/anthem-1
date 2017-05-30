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
class VariableStack
{
	public:
		using Layer = ast::VariableDeclarationPointers *;

	public:
		void push(Layer layer);
		void pop();

		std::experimental::optional<ast::VariableDeclaration *> findVariableDeclaration(const char *variableName) const;
		bool contains(const ast::VariableDeclaration &variableDeclaration) const;

	private:
		std::vector<Layer> m_layers;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::VariableDeclaration *> collectFreeVariables(ast::Formula &formula);
std::vector<ast::VariableDeclaration *> collectFreeVariables(ast::Formula &formula, ast::VariableStack &variableStack);

bool matches(const ast::Predicate &lhs, const ast::Predicate &rhs);
void collectPredicates(const ast::Formula &formula, std::vector<const ast::Predicate *> &predicates);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
