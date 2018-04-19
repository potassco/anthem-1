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

std::optional<VariableDeclaration *> VariableStack::findUserVariableDeclaration(const char *variableName) const
{
	const auto variableDeclarationMatches =
		[&variableName](const auto &variableDeclaration)
		{
			return variableDeclaration->type == VariableDeclaration::Type::UserDefined
				&& variableDeclaration->name == variableName;
		};

	for (auto i = m_layers.rbegin(); i != m_layers.rend(); i++)
	{
		auto &layer = **i;
		const auto matchingVariableDeclaration = std::find_if(layer.begin(), layer.end(), variableDeclarationMatches);

		if (matchingVariableDeclaration != layer.end())
			return matchingVariableDeclaration->get();
	}

	return std::nullopt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool VariableStack::contains(const VariableDeclaration &variableDeclaration) const
{
	const auto variableDeclarationMatches =
		[&variableDeclaration](const auto &other)
		{
			return &variableDeclaration == other.get();
		};

	const auto layerContainsVariableDeclaration =
		[&variableDeclarationMatches](const auto &layer)
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

	void visit(And &and_, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(Biconditional &biconditional, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		biconditional.left.accept(*this, variableStack, freeVariables);
		biconditional.right.accept(*this, variableStack, freeVariables);
	}

	void visit(Boolean &, VariableStack &, std::vector<VariableDeclaration *> &)
	{
	}

	void visit(Comparison &comparison, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		comparison.left.accept(*this, variableStack, freeVariables);
		comparison.right.accept(*this, variableStack, freeVariables);
	}

	void visit(Exists &exists, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		variableStack.push(&exists.variables);
		exists.argument.accept(*this, variableStack, freeVariables);
		variableStack.pop();
	}

	void visit(ForAll &forAll, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		variableStack.push(&forAll.variables);
		forAll.argument.accept(*this, variableStack, freeVariables);
		variableStack.pop();
	}

	void visit(Implies &implies, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		implies.antecedent.accept(*this, variableStack, freeVariables);
		implies.consequent.accept(*this, variableStack, freeVariables);
	}

	void visit(In &in, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		in.element.accept(*this, variableStack, freeVariables);
		in.set.accept(*this, variableStack, freeVariables);
	}

	void visit(Not &not_, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		not_.argument.accept(*this, variableStack, freeVariables);
	}

	void visit(Or &or_, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(Predicate &predicate, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Terms
	////////////////////////////////////////////////////////////////////////////////////////////////

	void visit(BinaryOperation &binaryOperation, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		binaryOperation.left.accept(*this, variableStack, freeVariables);
		binaryOperation.right.accept(*this, variableStack, freeVariables);
	}

	void visit(Function &function, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		for (auto &argument : function.arguments)
			argument.accept(*this, variableStack, freeVariables);
	}

	void visit(Integer &, VariableStack &, std::vector<VariableDeclaration *> &)
	{
	}

	void visit(Interval &interval, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		interval.from.accept(*this, variableStack, freeVariables);
		interval.to.accept(*this, variableStack, freeVariables);
	}

	void visit(SpecialInteger &, VariableStack &, std::vector<VariableDeclaration *> &)
	{
	}

	void visit(String &, VariableStack &, std::vector<VariableDeclaration *> &)
	{
	}

	void visit(UnaryOperation &unaryOperation, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		unaryOperation.argument.accept(*this, variableStack, freeVariables);
	}

	void visit(Variable &variable, VariableStack &variableStack, std::vector<VariableDeclaration *> &freeVariables)
	{
		if (variableStack.contains(*variable.declaration))
			return;

		if (std::find(freeVariables.cbegin(), freeVariables.cend(), variable.declaration) != freeVariables.cend())
			return;

		freeVariables.emplace_back(variable.declaration);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct CollectPredicateSignaturesVisitor : public RecursiveFormulaVisitor<CollectPredicateSignaturesVisitor>
{
	static void accept(const Predicate &predicate, const Formula &, std::vector<PredicateSignature> &predicateSignatures, Context &context)
	{
		const auto predicateSignatureMatches =
			[&predicate](const auto &predicateSignature)
			{
				return matches(predicate, predicateSignature);
			};

		if (std::find_if(predicateSignatures.cbegin(), predicateSignatures.cend(), predicateSignatureMatches) != predicateSignatures.cend())
			return;

		// TODO: avoid copies
		auto predicateSignature = PredicateSignature(std::string(predicate.name), predicate.arity());

		// Ignore predicates that are declared #external
		if (context.externalPredicateSignatures)
		{
			const auto matchesPredicateSignature =
				[&](const auto &otherPredicateSignature)
				{
					return ast::matches(predicateSignature, otherPredicateSignature.predicateSignature);
				};

			auto &externalPredicateSignatures = context.externalPredicateSignatures.value();

			const auto matchingExternalPredicateSignature =
				std::find_if(externalPredicateSignatures.begin(), externalPredicateSignatures.end(), matchesPredicateSignature);

			if (matchingExternalPredicateSignature != externalPredicateSignatures.end())
			{
				matchingExternalPredicateSignature->used = true;
				return;
			}
		}

		predicateSignatures.emplace_back(std::move(predicateSignature));
	}

	// Ignore all other types of expressions
	template<class T>
	static void accept(const T &, const Formula &, std::vector<PredicateSignature> &, const Context &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matches(const Predicate &lhs, const Predicate &rhs)
{
	return (lhs.name == rhs.name && lhs.arity() == rhs.arity());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matches(const Predicate &predicate, const PredicateSignature &signature)
{
	return (predicate.name == signature.name && predicate.arity() == signature.arity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool matches(const PredicateSignature &lhs, const PredicateSignature &rhs)
{
	return (lhs.name == rhs.name && lhs.arity == rhs.arity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: remove const_cast
void collectPredicateSignatures(const Formula &formula, std::vector<PredicateSignature> &predicateSignatures, Context &context)
{
	auto &formulaMutable = const_cast<Formula &>(formula);
	formulaMutable.accept(CollectPredicateSignaturesVisitor(), formulaMutable, predicateSignatures, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
