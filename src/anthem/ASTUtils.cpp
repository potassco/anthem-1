#include <anthem/ASTUtils.h>

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

void VariableStack::push(Layer layer)
{
	m_layers.push_back(layer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void VariableStack::pop()
{
	m_layers.pop_back();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool VariableStack::contains(const ast::Variable &variable) const
{
	const auto variableMatches =
		[&variable](const auto &otherVariable)
		{
			return variable.name == otherVariable.name;
		};

	const auto layerContainsVariable =
		[&variable, &variableMatches](const auto &layer)
		{
			return (std::find_if(layer->cbegin(), layer->cend(), variableMatches) != layer->cend());
		};

	return (std::find_if(m_layers.cbegin(), m_layers.cend(), layerContainsVariable) != m_layers.cend());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CollectFreeVariablesVisitor
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Formulas
	////////////////////////////////////////////////////////////////////////////////////////////////

	void visit(const ast::And &and_, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		for (const auto &argument : and_.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Biconditional &biconditional, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		biconditional.left.accept(*this, variableStack, freeVariables);
		biconditional.right.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Boolean &, VariableStack &, std::vector<ast::Variable> &)
	{
	}

	void visit(const ast::Comparison &comparison, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		comparison.left.accept(*this, variableStack, freeVariables);
		comparison.right.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Exists &exists, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		variableStack.push(&exists.variables);
		exists.argument.accept(*this, variableStack, freeVariables);
		variableStack.pop();
	}

	void visit(const ast::ForAll &forAll, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		variableStack.push(&forAll.variables);
		forAll.argument.accept(*this, variableStack, freeVariables);
		variableStack.pop();
	}

	void visit(const ast::Implies &implies, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		implies.antecedent.accept(*this, variableStack, freeVariables);
		implies.consequent.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::In &in, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		in.element.accept(*this, variableStack, freeVariables);
		in.set.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Not &not_, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		not_.argument.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Or &or_, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		for (const auto &argument : or_.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Predicate &predicate, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		for (const auto &argument : predicate.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Terms
	////////////////////////////////////////////////////////////////////////////////////////////////

	void visit(const ast::BinaryOperation &binaryOperation, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		binaryOperation.left.accept(*this, variableStack, freeVariables);
		binaryOperation.right.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Constant &, VariableStack &, std::vector<ast::Variable> &)
	{
	}

	void visit(const ast::Function &function, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		for (const auto &argument : function.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::Integer &, VariableStack &, std::vector<ast::Variable> &)
	{
	}

	void visit(const ast::Interval &interval, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		interval.from.accept(*this, variableStack, freeVariables);
		interval.to.accept(*this, variableStack, freeVariables);
	}

	void visit(const ast::SpecialInteger &, VariableStack &, std::vector<ast::Variable> &)
	{
	}

	void visit(const ast::String &, VariableStack &, std::vector<ast::Variable> &)
	{
	}

	void visit(const ast::Variable &variable, VariableStack &variableStack, std::vector<ast::Variable> &freeVariables)
	{
		if (variableStack.contains(variable))
			return;

		const auto &variableMatches =
			[&variable](auto &otherVariable)
			{
				return variable.name == otherVariable.name;
			};

		if (std::find_if(freeVariables.cbegin(), freeVariables.cend(), variableMatches) != freeVariables.cend())
			return;

		freeVariables.emplace_back(ast::deepCopy(variable));
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::Variable> collectFreeVariables(const ast::Formula &formula, ast::VariableStack &variableStack)
{
	std::vector<ast::Variable> freeVariables;

	formula.accept(CollectFreeVariablesVisitor(), variableStack, freeVariables);

	return freeVariables;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
