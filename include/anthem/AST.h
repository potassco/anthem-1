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

	BinaryOperation(Operator operator_, Term &&left, Term &&right)
	:	operator_{operator_},
		left{std::move(left)},
		right{std::move(right)}
	{
	}

	Operator operator_;
	Term left;
	Term right;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Boolean
{
	Boolean(bool value)
	:	value{value}
	{
	}

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

	Comparison(Operator operator_, Term &&left, Term &&right)
	:	operator_{operator_},
		left{std::move(left)},
		right{std::move(right)}
	{
	}

	Operator operator_;
	Term left;
	Term right;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Constant
{
	Constant(std::string &&name)
	:	name{std::move(name)}
	{
	}

	std::string name;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Function
{
	Function(std::string &&name)
	:	name{std::move(name)}
	{
	}

	Function(std::string &&name, std::vector<Term> &&arguments)
	:	name{std::move(name)},
		arguments{std::move(arguments)}
	{
	}

	std::string name;
	std::vector<Term> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct In
{
	In(Term &&element, Term &&set)
	:	element{std::move(element)},
		set{std::move(set)}
	{
	}

	Term element;
	Term set;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Integer
{
	Integer(int value)
	:	value{value}
	{
	}

	int value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Interval
{
	Interval(Term &&from, Term &&to)
	:	from{std::move(from)},
		to{std::move(to)}
	{
	}

	Term from;
	Term to;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Predicate
{
	Predicate(std::string &&name)
	:	name{std::move(name)}
	{
	}

	Predicate(std::string &&name, std::vector<Term> &&arguments)
	:	name{std::move(name)},
		arguments{std::move(arguments)}
	{
	}

	std::string name;
	std::vector<Term> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct SpecialInteger
{
	enum class Type
	{
		Infimum,
		Supremum
	};

	SpecialInteger(Type type)
	:	type{type}
	{
	}

	Type type;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct String
{
	String(std::string &&text)
	:	text{std::move(text)}
	{
	}

	std::string text;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Variable
{
	enum class Type
	{
		UserDefined,
		Reserved
	};

	Variable(std::string &&name, Type type)
	:	name{std::move(name)},
		type{type}
	{
	}

	std::string name;
	Type type;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Expressions
////////////////////////////////////////////////////////////////////////////////////////////////////

struct And
{
	And() = default;

	And(std::vector<Formula> &&arguments)
	:	arguments{std::move(arguments)}
	{
	}

	std::vector<Formula> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Biconditional
{
	Biconditional(Formula &&left, Formula &&right)
	:	left{std::move(left)},
		right{std::move(right)}
	{
	}

	Formula left;
	Formula right;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Exists
{
	Exists(std::vector<VariablePointer> &&variables, Formula &&argument)
	:	variables{std::move(variables)},
		argument{std::move(argument)}
	{
	}

	std::vector<VariablePointer> variables;
	Formula argument;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ForAll
{
	ForAll(std::vector<VariablePointer> &&variables, Formula &&argument)
	:	variables{std::move(variables)},
		argument{std::move(argument)}
	{
	}

	std::vector<VariablePointer> variables;
	Formula argument;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Implies
{
	Implies(Formula &&antecedent, Formula &&consequent)
	:	antecedent{std::move(antecedent)},
		consequent{std::move(consequent)}
	{
	}

	Formula antecedent;
	Formula consequent;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Not
{
	Not(Formula &&argument)
	:	argument{std::move(argument)}
	{
	}

	Formula argument;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Or
{
	Or() = default;

	Or(std::vector<Formula> &&arguments)
	:	arguments{std::move(arguments)}
	{
	}

	std::vector<Formula> arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Deep Copying
////////////////////////////////////////////////////////////////////////////////////////////////////

BinaryOperation deepCopy(const BinaryOperation &other);
Boolean deepCopy(const Boolean &other);
Comparison deepCopy(const Comparison &other);
Constant deepCopy(const Constant &other);
Function deepCopy(const Function &other);
Integer deepCopy(const Integer &other);
Interval deepCopy(const Interval &other);
Predicate deepCopy(const Predicate &other);
SpecialInteger deepCopy(const SpecialInteger &other);
String deepCopy(const String &other);
Variable deepCopy(const Variable &other);
std::vector<VariablePointer> deepCopy(const std::vector<VariablePointer> &other);
And deepCopy(const And &other);
Biconditional deepCopy(const Biconditional &other);
Exists deepCopy(const Exists &other);
ForAll deepCopy(const ForAll &other);
Implies deepCopy(const Implies &other);
Not deepCopy(const Not &other);
Or deepCopy(const Or &other);

Formula deepCopy(const Formula &formula);
std::vector<Formula> deepCopy(const std::vector<Formula> &formulas);
Term deepCopy(const Term &term);
std::vector<Term> deepCopy(const std::vector<Term> &terms);

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto deepCopyUniquePtr =
	[](const auto &uniquePtr) -> typename std::decay<decltype(uniquePtr)>::type
	{
		using Type = typename std::decay<decltype(uniquePtr)>::type::element_type;
		return std::make_unique<Type>(deepCopy(*uniquePtr));
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto deepCopyUniquePtrVector =
	[](const auto &uniquePtrVector) -> typename std::decay<decltype(uniquePtrVector)>::type
	{
		using Type = typename std::decay<decltype(uniquePtrVector)>::type::value_type;

		std::vector<Type> result;
		result.reserve(uniquePtrVector.size());

		for (const auto &uniquePtr : uniquePtrVector)
			result.emplace_back(deepCopyUniquePtr(uniquePtr));

		return result;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto deepCopyVariant =
	[](const auto &variant) -> typename std::decay<decltype(variant)>::type
	{
		return variant.match([](const auto &x) -> typename std::decay<decltype(variant)>::type {return deepCopyUniquePtr(x);});
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto deepCopyVariantVector =
	[](const auto &variantVector) -> typename std::decay<decltype(variantVector)>::type
	{
		using Type = typename std::decay<decltype(variantVector)>::type::value_type;

		std::vector<Type> result;
		result.reserve(variantVector.size());

		for (const auto &variant : variantVector)
			result.emplace_back(deepCopyVariant(variant));

		return result;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

BinaryOperation deepCopy(const BinaryOperation &other)
{
	return BinaryOperation(other.operator_, deepCopy(other.left), deepCopy(other.right));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Boolean deepCopy(const Boolean &other)
{
	return Boolean(other.value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Comparison deepCopy(const Comparison &other)
{
	return Comparison(other.operator_, deepCopy(other.left), deepCopy(other.right));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Constant deepCopy(const Constant &other)
{
	return Constant(std::string(other.name));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Function deepCopy(const Function &other)
{
	return Function(std::string(other.name), deepCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Integer deepCopy(const Integer &other)
{
	return Integer(other.value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Interval deepCopy(const Interval &other)
{
	return Interval(deepCopy(other.from), deepCopy(other.to));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Predicate deepCopy(const Predicate &other)
{
	return Predicate(std::string(other.name), deepCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SpecialInteger deepCopy(const SpecialInteger &other)
{
	return SpecialInteger(other.type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

String deepCopy(const String &other)
{
	return String(std::string(other.text));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Variable deepCopy(const Variable &other)
{
	return Variable(std::string(other.name), other.type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<VariablePointer> deepCopy(const std::vector<VariablePointer> &other)
{
	return deepCopyUniquePtrVector(other);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

And deepCopy(const And &other)
{
	return And(deepCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Biconditional deepCopy(const Biconditional &other)
{
	return Biconditional(deepCopy(other.left), deepCopy(other.right));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Exists deepCopy(const Exists &other)
{
	return Exists(deepCopy(other.variables), deepCopy(other.argument));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ForAll deepCopy(const ForAll &other)
{
	return ForAll(deepCopy(other.variables), deepCopy(other.argument));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

In deepCopy(const In &other)
{
	return In(deepCopy(other.element), deepCopy(other.set));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Implies deepCopy(const Implies &other)
{
	return Implies(deepCopy(other.antecedent), deepCopy(other.consequent));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Not deepCopy(const Not &other)
{
	return Not(deepCopy(other.argument));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Or deepCopy(const Or &other)
{
	return Or(deepCopy(other.arguments));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Formula deepCopy(const Formula &formula)
{
	return deepCopyVariant(formula);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Formula> deepCopy(const std::vector<Formula> &formulas)
{
	return deepCopyVariantVector(formulas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Term deepCopy(const Term &term)
{
	return deepCopyVariant(term);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Term> deepCopy(const std::vector<Term> &terms)
{
	return deepCopyVariantVector(terms);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
