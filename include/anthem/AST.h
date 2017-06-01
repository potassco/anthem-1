#ifndef __ANTHEM__AST_H
#define __ANTHEM__AST_H

#include <anthem/ASTForward.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Terms are primitive (or arguments) if they are neither operations nor intervals
inline bool isPrimitive(const Term &term)
{
	return (!term.is<BinaryOperation>() && !term.is<Interval>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Primitives
////////////////////////////////////////////////////////////////////////////////////////////////////

struct BinaryOperation
{
	enum class Operator
	{
		Plus,
		Minus,
		Multiplication,
		Division,
		Modulo
	};

	explicit BinaryOperation(Operator operator_, Term &&left, Term &&right)
	:	operator_{operator_},
		left{std::move(left)},
		right{std::move(right)}
	{
	}

	BinaryOperation(const BinaryOperation &other) = delete;
	BinaryOperation &operator=(const BinaryOperation &other) = delete;
	BinaryOperation(BinaryOperation &&other) noexcept = default;
	BinaryOperation &operator=(BinaryOperation &&other) noexcept = default;

	Operator operator_;
	Term left;
	Term right;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Boolean
{
	explicit Boolean(bool value)
	:	value{value}
	{
	}

	Boolean(const Boolean &other) = delete;
	Boolean &operator=(const Boolean &other) = delete;
	Boolean(Boolean &&other) noexcept = default;
	Boolean &operator=(Boolean &&other) noexcept = default;

	bool value = false;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Comparison
{
	enum class Operator
	{
		GreaterThan,
		LessThan,
		LessEqual,
		GreaterEqual,
		NotEqual,
		Equal
	};

	explicit Comparison(Operator operator_, Term &&left, Term &&right)
	:	operator_{operator_},
		left{std::move(left)},
		right{std::move(right)}
	{
	}

	Comparison(const Comparison &other) = delete;
	Comparison &operator=(const Comparison &other) = delete;
	Comparison(Comparison &&other) noexcept = default;
	Comparison &operator=(Comparison &&other) noexcept = default;

	Operator operator_;
	Term left;
	Term right;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Constant
{
	explicit Constant(std::string &&name)
	:	name{std::move(name)}
	{
	}

	Constant(const Constant &other) = delete;
	Constant &operator=(const Constant &other) = delete;
	Constant(Constant &&other) = default;
	Constant &operator=(Constant &&other) = default;

	std::string name;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Function
{
	explicit Function(std::string &&name)
	:	name{std::move(name)}
	{
	}

	explicit Function(std::string &&name, std::vector<Term> &&arguments)
	:	name{std::move(name)},
		arguments{std::move(arguments)}
	{
	}

	Function(const Function &other) = delete;
	Function &operator=(const Function &other) = delete;
	Function(Function &&other) noexcept = default;
	Function &operator=(Function &&other) noexcept = default;

	std::string name;
	std::vector<Term> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: refactor (limit element type to primitive terms)
struct In
{
	explicit In(Term &&element, Term &&set)
	:	element{std::move(element)},
		set{std::move(set)}
	{
		// While the set may be any term, the element must be primitive
		assert(isPrimitive(element));
	}

	In(const In &other) = delete;
	In &operator=(const In &other) = delete;
	In(In &&other) noexcept = default;
	In &operator=(In &&other) noexcept = default;

	Term element;
	Term set;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Integer
{
	explicit Integer(int value)
	:	value{value}
	{
	}

	Integer(const Integer &other) = delete;
	Integer &operator=(const Integer &other) = delete;
	Integer(Integer &&other) noexcept = default;
	Integer &operator=(Integer &&other) noexcept = default;

	int value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Interval
{
	explicit Interval(Term &&from, Term &&to)
	:	from{std::move(from)},
		to{std::move(to)}
	{
	}

	Interval(const Interval &other) = delete;
	Interval &operator=(const Interval &other) = delete;
	Interval(Interval &&other) noexcept = default;
	Interval &operator=(Interval &&other) noexcept = default;

	Term from;
	Term to;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Predicate
{
	explicit Predicate(std::string &&name)
	:	name{std::move(name)}
	{
	}

	explicit Predicate(std::string &&name, std::vector<Term> &&arguments)
	:	name{std::move(name)},
		arguments{std::move(arguments)}
	{
	}

	Predicate(const Predicate &other) = delete;
	Predicate &operator=(const Predicate &other) = delete;
	Predicate(Predicate &&other) noexcept = default;
	Predicate &operator=(Predicate &&other) noexcept = default;

	std::size_t arity() const
	{
		return arguments.size();
	}

	std::string name;
	std::vector<Term> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: make more use of this class
struct PredicateSignature
{
	explicit PredicateSignature(std::string &&name, size_t arity)
	:	name{std::move(name)},
		arity{arity}
	{
	}

	PredicateSignature(const PredicateSignature &other) = delete;
	PredicateSignature &operator=(const PredicateSignature &other) = delete;
	PredicateSignature(PredicateSignature &&other) noexcept = default;
	PredicateSignature &operator=(PredicateSignature &&other) noexcept = default;

	std::string name;
	size_t arity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SpecialInteger
{
	enum class Type
	{
		Infimum,
		Supremum
	};

	explicit SpecialInteger(Type type)
	:	type{type}
	{
	}

	SpecialInteger(const SpecialInteger &other) = delete;
	SpecialInteger &operator=(const SpecialInteger &other) = delete;
	SpecialInteger(SpecialInteger &&other) noexcept = default;
	SpecialInteger &operator=(SpecialInteger &&other) noexcept = default;

	Type type;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct String
{
	explicit String(std::string &&text)
	:	text{std::move(text)}
	{
	}

	String(const String &other) = delete;
	String &operator=(const String &other) = delete;
	String(String &&other) noexcept = default;
	String &operator=(String &&other) noexcept = default;

	std::string text;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Variable
{
	explicit Variable(VariableDeclaration *declaration)
	:	declaration{declaration}
	{
	}

	Variable(const Variable &other) = delete;
	Variable &operator=(const Variable &other) = delete;
	Variable(Variable &&other) = default;
	Variable &operator=(Variable &&other) = default;

	VariableDeclaration *declaration = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct VariableDeclaration
{
	enum class Type
	{
		UserDefined,
		Head,
		Body
	};

	explicit VariableDeclaration(Type type)
	:	type{type}
	{
	}

	explicit VariableDeclaration(Type type, std::string &&name)
	:	type{type},
		name{std::move(name)}
	{
	}

	VariableDeclaration(const VariableDeclaration &other) = delete;
	VariableDeclaration &operator=(const VariableDeclaration &other) = delete;
	VariableDeclaration(VariableDeclaration &&other) = default;
	VariableDeclaration &operator=(VariableDeclaration &&other) = default;

	Type type;
	std::string name;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Expressions
////////////////////////////////////////////////////////////////////////////////////////////////////

struct And
{
	And() = default;

	explicit And(std::vector<Formula> &&arguments)
	:	arguments{std::move(arguments)}
	{
	}

	And(const And &other) = delete;
	And &operator=(const And &other) = delete;
	And(And &&other) noexcept = default;
	And &operator=(And &&other) noexcept = default;

	std::vector<Formula> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Biconditional
{
	explicit Biconditional(Formula &&left, Formula &&right)
	:	left{std::move(left)},
		right{std::move(right)}
	{
	}

	Biconditional(const Biconditional &other) = delete;
	Biconditional &operator=(const Biconditional &other) = delete;
	Biconditional(Biconditional &&other) noexcept = default;
	Biconditional &operator=(Biconditional &&other) noexcept = default;

	Formula left;
	Formula right;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Exists
{
	// TODO: rename “variables”
	explicit Exists(VariableDeclarationPointers &&variables, Formula &&argument)
	:	variables{std::move(variables)},
		argument{std::move(argument)}
	{
	}

	Exists(const Exists &other) = delete;
	Exists &operator=(const Exists &other) = delete;
	Exists(Exists &&other) noexcept = default;
	Exists &operator=(Exists &&other) noexcept = default;

	VariableDeclarationPointers variables;
	Formula argument;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ForAll
{
	explicit ForAll(VariableDeclarationPointers &&variables, Formula &&argument)
	:	variables{std::move(variables)},
		argument{std::move(argument)}
	{
	}

	ForAll(const ForAll &other) = delete;
	ForAll &operator=(const ForAll &other) = delete;
	ForAll(ForAll &&other) noexcept = default;
	ForAll &operator=(ForAll &&other) noexcept = default;

	VariableDeclarationPointers variables;
	Formula argument;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Implies
{
	explicit Implies(Formula &&antecedent, Formula &&consequent)
	:	antecedent{std::move(antecedent)},
		consequent{std::move(consequent)}
	{
	}

	Implies(const Implies &other) = delete;
	Implies &operator=(const Implies &other) = delete;
	Implies(Implies &&other) noexcept = default;
	Implies &operator=(Implies &&other) noexcept = default;

	Formula antecedent;
	Formula consequent;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Not
{
	explicit Not(Formula &&argument)
	:	argument{std::move(argument)}
	{
	}

	Not(const Not &other) = delete;
	Not &operator=(const Not &other) = delete;
	Not(Not &&other) noexcept = default;
	Not &operator=(Not &&other) noexcept = default;

	Formula argument;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Or
{
	Or() = default;

	explicit Or(std::vector<Formula> &&arguments)
	:	arguments{std::move(arguments)}
	{
	}

	Or(const Or &other) = delete;
	Or &operator=(const Or &other) = delete;
	Or(Or &&other) noexcept = default;
	Or &operator=(Or &&other) noexcept = default;

	std::vector<Formula> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// High-Level
////////////////////////////////////////////////////////////////////////////////////////////////////

struct ScopedFormula
{
	explicit ScopedFormula(Formula &&formula, VariableDeclarationPointers &&freeVariables)
	:	formula{std::move(formula)},
		freeVariables{std::move(freeVariables)}
	{
	}

	Formula formula;
	VariableDeclarationPointers freeVariables;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
