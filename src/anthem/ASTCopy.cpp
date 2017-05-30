#include <anthem/ASTCopy.h>

#include <map>

#include <anthem/ASTUtils.h>
#include <anthem/ASTVisitors.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ASTCopy
//
////////////////////////////////////////////////////////////////////////////////////////////////////

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
// Preparing Copying
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Variant>
struct VariantDeepCopyVisitor
{
	template<class T>
	Variant visit(const T &x)
	{
		return prepareCopy(x);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
std::unique_ptr<T> prepareCopy(const std::unique_ptr<T> &uniquePtr)
{
	return std::make_unique<T>(prepareCopy(*uniquePtr));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto prepareCopyVariant =
	[](const auto &variant) -> typename std::decay<decltype(variant)>::type
	{
		using VariantType = typename std::decay<decltype(variant)>::type;

		return variant.accept(VariantDeepCopyVisitor<VariantType>());
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto prepareCopyVariantVector =
	[](const auto &variantVector) -> typename std::decay<decltype(variantVector)>::type
	{
		using Type = typename std::decay<decltype(variantVector)>::type::value_type;

		std::vector<Type> result;
		result.reserve(variantVector.size());

		for (const auto &variant : variantVector)
			result.emplace_back(prepareCopyVariant(variant));

		return result;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto prepareCopyVector =
	[](const auto &vector) -> typename std::decay<decltype(vector)>::type
	{
		using Type = typename std::decay<decltype(vector)>::type::value_type;

		std::vector<Type> result;
		result.reserve(vector.size());

		for (const auto &element : vector)
			result.emplace_back(prepareCopy(element));

		return result;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

BinaryOperation prepareCopy(const BinaryOperation &other)
{
	return BinaryOperation(other.operator_, prepareCopy(other.left), prepareCopy(other.right));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Boolean prepareCopy(const Boolean &other)
{
	return Boolean(other.value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Comparison prepareCopy(const Comparison &other)
{
	return Comparison(other.operator_, prepareCopy(other.left), prepareCopy(other.right));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Constant prepareCopy(const Constant &other)
{
	return Constant(std::string(other.name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Function prepareCopy(const Function &other)
{
	return Function(std::string(other.name), prepareCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

In prepareCopy(const In &other)
{
	return In(prepareCopy(other.element), prepareCopy(other.set));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Integer prepareCopy(const Integer &other)
{
	return Integer(other.value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Interval prepareCopy(const Interval &other)
{
	return Interval(prepareCopy(other.from), prepareCopy(other.to));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Predicate prepareCopy(const Predicate &other)
{
	return Predicate(std::string(other.name), prepareCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SpecialInteger prepareCopy(const SpecialInteger &other)
{
	return SpecialInteger(other.type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

String prepareCopy(const String &other)
{
	return String(std::string(other.text));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Variable prepareCopy(const Variable &other)
{
	return Variable(other.declaration);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

VariableDeclaration prepareCopy(const VariableDeclaration &other)
{
	return VariableDeclaration(other.type, std::string(other.name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

VariableDeclarationPointers prepareCopy(const VariableDeclarationPointers &other)
{
	return prepareCopyVector(other);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

And prepareCopy(const And &other)
{
	return And(prepareCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Biconditional prepareCopy(const Biconditional &other)
{
	return Biconditional(prepareCopy(other.left), prepareCopy(other.right));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Exists prepareCopy(const Exists &other)
{
	Exists copy(prepareCopy(other.variables), prepareCopy(other.argument));

	// TODO: refactor
	for (size_t i = 0; i < other.variables.size(); i++)
		copy.argument.accept(ReplaceVariableInFormulaVisitor(), copy.argument, other.variables[i].get(), copy.variables[i].get());

	return copy;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ForAll prepareCopy(const ForAll &other)
{
	ForAll copy(prepareCopy(other.variables), prepareCopy(other.argument));

	// TODO: refactor
	for (size_t i = 0; i < other.variables.size(); i++)
		copy.argument.accept(ReplaceVariableInFormulaVisitor(), copy.argument, other.variables[i].get(), copy.variables[i].get());

	return copy;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Implies prepareCopy(const Implies &other)
{
	return Implies(prepareCopy(other.antecedent), prepareCopy(other.consequent));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Not prepareCopy(const Not &other)
{
	return Not(prepareCopy(other.argument));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Or prepareCopy(const Or &other)
{
	return Or(prepareCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Formula prepareCopy(const Formula &formula)
{
	return prepareCopyVariant(formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Term prepareCopy(const Term &term)
{
	return prepareCopyVariant(term);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Term> prepareCopy(const std::vector<Term> &terms)
{
	return prepareCopyVariantVector(terms);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Formula> prepareCopy(const std::vector<Formula> &formulas)
{
	return prepareCopyVariantVector(formulas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Fixing Dangling Variables
////////////////////////////////////////////////////////////////////////////////////////////////////

// Fix all dangling variables in a given term
struct FixDanglingVariablesInTermVisitor
{
	template <class... Arguments>
	void visit(BinaryOperation &binaryOperation, Arguments &&... arguments)
	{
		binaryOperation.left.accept(*this, std::forward<Arguments>(arguments)...);
		binaryOperation.right.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Boolean &, Arguments &&...)
	{
	}

	template <class... Arguments>
	void visit(Constant &, Arguments &&...)
	{
	}

	template <class... Arguments>
	void visit(Function &function, Arguments &&... arguments)
	{
		for (auto &argument : function.arguments)
			argument.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Integer &, Arguments &&...)
	{
	}

	template <class... Arguments>
	void visit(Interval &interval, Arguments &&... arguments)
	{
		interval.from.accept(*this, std::forward<Arguments>(arguments)...);
		interval.to.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(SpecialInteger &, Arguments &&...)
	{
	}

	template <class... Arguments>
	void visit(String &, Arguments &&...)
	{
	}

	void visit(Variable &variable, ScopedFormula &scopedFormula, VariableStack &variableStack,
		std::map<VariableDeclaration *, VariableDeclaration *> &replacements)
	{
		const auto match = replacements.find(variable.declaration);

		// Replace the variable if it is flagged for replacement
		if (match != replacements.cend())
		{
			variable.declaration = match->second;
			return;
		}

		// If the variable is not flagged for replacement yet, check whether it is dangling
		const auto isVariableDangling = !variableStack.contains(*variable.declaration);

		if (!isVariableDangling)
			return;

		// If the variable is dangling, declare it correctly and flag it for future replacement
		auto newVariableDeclaration = std::make_unique<VariableDeclaration>(variable.declaration->type, std::string(variable.declaration->name));
		scopedFormula.freeVariables.emplace_back(std::move(newVariableDeclaration));

		replacements[variable.declaration] = scopedFormula.freeVariables.back().get();
		variable.declaration = scopedFormula.freeVariables.back().get();
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Fix all dangling variables in a given formula
struct FixDanglingVariablesInFormulaVisitor
{
	template <class... Arguments>
	void visit(And &and_, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			argument.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Biconditional &biconditional, Arguments &&... arguments)
	{
		biconditional.left.accept(*this, std::forward<Arguments>(arguments)...);
		biconditional.right.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Boolean &, Arguments &&...)
	{
	}

	template <class... Arguments>
	void visit(Comparison &comparison, Arguments &&... arguments)
	{
		comparison.left.accept(FixDanglingVariablesInTermVisitor(), std::forward<Arguments>(arguments)...);
		comparison.right.accept(FixDanglingVariablesInTermVisitor(), std::forward<Arguments>(arguments)...);
	}

	void visit(Exists &exists, ScopedFormula &scopedFormula, VariableStack &variableStack,
		std::map<VariableDeclaration *, VariableDeclaration *> &replacements)
	{
		variableStack.push(&exists.variables);
		exists.argument.accept(*this, scopedFormula, variableStack, replacements);
		variableStack.pop();
	}

	void visit(ForAll &forAll, ScopedFormula &scopedFormula, VariableStack &variableStack,
		std::map<VariableDeclaration *, VariableDeclaration *> &replacements)
	{
		variableStack.push(&forAll.variables);
		forAll.argument.accept(*this, scopedFormula, variableStack, replacements);
		variableStack.pop();
	}

	template <class... Arguments>
	void visit(Implies &implies, Arguments &&... arguments)
	{
		implies.antecedent.accept(*this, std::forward<Arguments>(arguments)...);
		implies.consequent.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(In &in, Arguments &&... arguments)
	{
		in.element.accept(FixDanglingVariablesInTermVisitor(), std::forward<Arguments>(arguments)...);
		in.set.accept(FixDanglingVariablesInTermVisitor(), std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Not &not_, Arguments &&... arguments)
	{
		not_.argument.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Or &or_, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			argument.accept(*this, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	void visit(Predicate &predicate, Arguments &&... arguments)
	{
		for (auto &argument : predicate.arguments)
			argument.accept(FixDanglingVariablesInTermVisitor(), std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void fixDanglingVariables(ScopedFormula &scopedFormula)
{
	VariableStack variableStack;
	variableStack.push(&scopedFormula.freeVariables);

	std::map<VariableDeclaration *, VariableDeclaration *> replacements;

	scopedFormula.formula.accept(FixDanglingVariablesInFormulaVisitor(), scopedFormula,
		variableStack, replacements);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
