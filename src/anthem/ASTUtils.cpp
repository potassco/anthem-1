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

std::experimental::optional<ast::VariableDeclaration *> VariableStack::findUserVariableDeclaration(const char *variableName) const
{
	const auto variableNameMatches =
		[&variableName](const auto &variableDeclaration)
		{
			return variableDeclaration->type == VariableDeclaration::Type::UserDefined
				&& variableDeclaration->name == variableName;
		};

	for (auto i = m_layers.rbegin(); i != m_layers.rend(); i++)
	{
		auto &layer = **i;
		const auto matchingVariableDeclaration = std::find_if(layer.begin(), layer.end(), variableNameMatches);

		if (matchingVariableDeclaration != layer.end())
			return matchingVariableDeclaration->get();
	}

	return std::experimental::nullopt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool VariableStack::contains(const ast::VariableDeclaration &variableDeclaration) const
{
	const auto variableDeclarationMatches =
		[&variableDeclaration](const auto &other)
		{
			return &variableDeclaration == other.get();
		};

	const auto layerContainsVariableDeclaration =
		[&variableDeclaration, &variableDeclarationMatches](const auto &layer)
		{
			return (std::find_if(layer->cbegin(), layer->cend(), variableDeclarationMatches) != layer->cend());
		};

	return (std::find_if(m_layers.cbegin(), m_layers.cend(), layerContainsVariableDeclaration) != m_layers.cend());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CollectFreeVariablesVisitor
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Formulas
	////////////////////////////////////////////////////////////////////////////////////////////////

	void visit(ast::And &and_, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Biconditional &biconditional, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		biconditional.left.accept(*this, variableStack, freeVariables);
		biconditional.right.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Boolean &, VariableStack &, std::vector<ast::VariableDeclaration *> &)
	{
	}

	void visit(ast::Comparison &comparison, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		comparison.left.accept(*this, variableStack, freeVariables);
		comparison.right.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Exists &exists, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		variableStack.push(&exists.variables);
		exists.argument.accept(*this, variableStack, freeVariables);
		variableStack.pop();
	}

	void visit(ast::ForAll &forAll, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		variableStack.push(&forAll.variables);
		forAll.argument.accept(*this, variableStack, freeVariables);
		variableStack.pop();
	}

	void visit(ast::Implies &implies, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		implies.antecedent.accept(*this, variableStack, freeVariables);
		implies.consequent.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::In &in, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		in.element.accept(*this, variableStack, freeVariables);
		in.set.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Not &not_, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		not_.argument.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Or &or_, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Predicate &predicate, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Terms
	////////////////////////////////////////////////////////////////////////////////////////////////

	void visit(ast::BinaryOperation &binaryOperation, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		binaryOperation.left.accept(*this, variableStack, freeVariables);
		binaryOperation.right.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Constant &, VariableStack &, std::vector<ast::VariableDeclaration *> &)
	{
	}

	void visit(ast::Function &function, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : function.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::Integer &, VariableStack &, std::vector<ast::VariableDeclaration *> &)
	{
	}

	void visit(ast::Interval &interval, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		interval.from.accept(*this, variableStack, freeVariables);
		interval.to.accept(*this, variableStack, freeVariables);
	}

	void visit(ast::SpecialInteger &, VariableStack &, std::vector<ast::VariableDeclaration *> &)
	{
	}

	void visit(ast::String &, VariableStack &, std::vector<ast::VariableDeclaration *> &)
	{
	}

	void visit(ast::Variable &variable, VariableStack &variableStack, std::vector<ast::VariableDeclaration *> &freeVariables)
	{
		if (variableStack.contains(*variable.declaration))
			return;

		if (std::find(freeVariables.cbegin(), freeVariables.cend(), variable.declaration) != freeVariables.cend())
			return;

		freeVariables.emplace_back(variable.declaration);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::VariableDeclaration *> collectFreeVariables(ast::Formula &formula)
{
	ast::VariableStack variableStack;
	return collectFreeVariables(formula, variableStack);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ast::VariableDeclaration *> collectFreeVariables(ast::Formula &formula, ast::VariableStack &variableStack)
{
	std::vector<ast::VariableDeclaration *> freeVariables;

	formula.accept(CollectFreeVariablesVisitor(), variableStack, freeVariables);

	return freeVariables;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CollectPredicatesVisitor : public RecursiveFormulaVisitor<CollectPredicatesVisitor>
{
	static void accept(const ast::Predicate &predicate, const ast::Formula &, std::vector<const ast::Predicate *> &predicates)
	{
		const auto predicateMatches =
			[&predicate](const auto *otherPredicate)
			{
				return matches(predicate, *otherPredicate);
			};

		if (std::find_if(predicates.cbegin(), predicates.cend(), predicateMatches) == predicates.cend())
			predicates.emplace_back(&predicate);
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(const T &, const ast::Formula &, std::vector<const ast::Predicate *> &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matches(const ast::Predicate &lhs, const ast::Predicate &rhs)
{
	return (lhs.name == rhs.name && lhs.arity() == rhs.arity());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: remove const_cast
void collectPredicates(const ast::Formula &formula, std::vector<const ast::Predicate *> &predicates)
{
	auto &formulaMutable = const_cast<ast::Formula &>(formula);
	formulaMutable.accept(CollectPredicatesVisitor(), formulaMutable, predicates);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
