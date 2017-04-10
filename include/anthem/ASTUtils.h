#ifndef __ANTHEM__AST_UTILS_H
#define __ANTHEM__AST_UTILS_H

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

class VariableStack
{
	public:
		using Layer = const std::vector<ast::Variable> *;

	public:
		void push(Layer layer);
		void pop();

		bool contains(const ast::Variable &variable) const;

	private:
		std::vector<Layer> m_layers;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::Variable> collectFreeVariables(const ast::Formula &formula);
std::vector<ast::Variable> collectFreeVariables(const ast::Formula &formula, ast::VariableStack &variableStack);

bool matches(const ast::Predicate &lhs, const ast::Predicate &rhs);
void collectPredicates(const ast::Formula &formula, std::vector<const ast::Predicate *> &predicates);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
