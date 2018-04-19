#ifndef __ANTHEM__EQUALITY_H
#define __ANTHEM__EQUALITY_H

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Equality
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: move to separate class
enum class Tristate
{
	True,
	False,
	Unknown,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

Tristate equal(const Formula &lhs, const Formula &rhs);
Tristate equal(const Term &lhs, const Term &rhs);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormulaEqualityVisitor
{
	Tristate visit(const And &and_, const Formula &otherFormula)
	{
		if (!otherFormula.is<And>())
			return Tristate::Unknown;

		const auto &otherAnd = otherFormula.get<And>();

		for (const auto &argument : and_.arguments)
		{
			const auto match = std::find_if(
				otherAnd.arguments.cbegin(), otherAnd.arguments.cend(),
				[&](const auto &otherArgument)
				{
					return equal(argument, otherArgument) == Tristate::True;
				});

			if (match == otherAnd.arguments.cend())
				return Tristate::Unknown;
		}

		for (const auto &otherArgument : otherAnd.arguments)
		{
			const auto match = std::find_if(
				and_.arguments.cbegin(), and_.arguments.cend(),
				[&](const auto &argument)
				{
					return equal(otherArgument, argument) == Tristate::True;
				});

			if (match == and_.arguments.cend())
				return Tristate::Unknown;
		}

		return Tristate::True;
	}

	Tristate visit(const Biconditional &biconditional, const Formula &otherFormula)
	{
		if (!otherFormula.is<Biconditional>())
			return Tristate::Unknown;

		const auto &otherBiconditional = otherFormula.get<Biconditional>();

		if (equal(biconditional.left, otherBiconditional.left) == Tristate::True
		    && equal(biconditional.right, otherBiconditional.right) == Tristate::True)
		{
			return Tristate::True;
		}

		if (equal(biconditional.left, otherBiconditional.right) == Tristate::True
		    && equal(biconditional.right, otherBiconditional.left) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const Boolean &boolean, const Formula &otherFormula)
	{
		if (!otherFormula.is<Boolean>())
			return Tristate::Unknown;

		const auto &otherBoolean = otherFormula.get<Boolean>();

		return (boolean.value == otherBoolean.value)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const Comparison &comparison, const Formula &otherFormula)
	{
		if (!otherFormula.is<Comparison>())
			return Tristate::Unknown;

		const auto &otherComparison = otherFormula.get<Comparison>();

		if (comparison.operator_ != otherComparison.operator_)
			return Tristate::Unknown;

		if (equal(comparison.left, otherComparison.left) == Tristate::True
		    && equal(comparison.right, otherComparison.right) == Tristate::True)
		{
			return Tristate::True;
		}

		// Only = and != are commutative operators, all others don’t need to be checked with exchanged arguments
		if (comparison.operator_ != Comparison::Operator::Equal
		    && comparison.operator_ != Comparison::Operator::NotEqual)
		{
			return Tristate::Unknown;
		}

		if (equal(comparison.left, otherComparison.right) == Tristate::True
		    && equal(comparison.right, otherComparison.left) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const Exists &, const Formula &otherFormula)
	{
		if (!otherFormula.is<Exists>())
			return Tristate::Unknown;

		// TODO: implement stronger check
		return Tristate::Unknown;
	}

	Tristate visit(const ForAll &, const Formula &otherFormula)
	{
		if (!otherFormula.is<ForAll>())
			return Tristate::Unknown;

		// TODO: implement stronger check
		return Tristate::Unknown;
	}

	Tristate visit(const Implies &implies, const Formula &otherFormula)
	{
		if (!otherFormula.is<Implies>())
			return Tristate::Unknown;

		const auto &otherImplies = otherFormula.get<Implies>();

		if (equal(implies.antecedent, otherImplies.antecedent) == Tristate::True
		    && equal(implies.consequent, otherImplies.consequent) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const In &in, const Formula &otherFormula)
	{
		if (!otherFormula.is<In>())
			return Tristate::Unknown;

		const auto &otherIn = otherFormula.get<In>();

		if (equal(in.element, otherIn.element) == Tristate::True
		    && equal(in.set, otherIn.set) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const Not &not_, const Formula &otherFormula)
	{
		if (!otherFormula.is<Not>())
			return Tristate::Unknown;

		const auto &otherNot = otherFormula.get<Not>();

		return equal(not_.argument, otherNot.argument);
	}

	Tristate visit(const Or &or_, const Formula &otherFormula)
	{
		if (!otherFormula.is<Or>())
			return Tristate::Unknown;

		const auto &otherOr = otherFormula.get<Or>();

		for (const auto &argument : or_.arguments)
		{
			const auto match = std::find_if(
				otherOr.arguments.cbegin(), otherOr.arguments.cend(),
				[&](const auto &otherArgument)
				{
					return equal(argument, otherArgument) == Tristate::True;
				});

			if (match == otherOr.arguments.cend())
				return Tristate::Unknown;
		}

		for (const auto &otherArgument : otherOr.arguments)
		{
			const auto match = std::find_if(
				or_.arguments.cbegin(), or_.arguments.cend(),
				[&](const auto &argument)
				{
					return equal(otherArgument, argument) == Tristate::True;
				});

			if (match == or_.arguments.cend())
				return Tristate::Unknown;
		}

		return Tristate::True;
	}

	Tristate visit(const Predicate &predicate, const Formula &otherFormula)
	{
		if (!otherFormula.is<Predicate>())
			return Tristate::Unknown;

		const auto &otherPredicate = otherFormula.get<Predicate>();

		if (predicate.declaration != otherPredicate.declaration)
			return Tristate::False;

		assert(predicate.arguments.size() == otherPredicate.arguments.size());

		for (size_t i = 0; i < predicate.arguments.size(); i++)
			if (equal(predicate.arguments[i], otherPredicate.arguments[i]) != Tristate::True)
				return Tristate::Unknown;

		return Tristate::True;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermEqualityVisitor
{
	Tristate visit(const BinaryOperation &binaryOperation, const Term &otherTerm)
	{
		if (!otherTerm.is<BinaryOperation>())
			return Tristate::Unknown;

		const auto &otherBinaryOperation = otherTerm.get<BinaryOperation>();

		if (binaryOperation.operator_ != otherBinaryOperation.operator_)
			return Tristate::Unknown;

		if (equal(binaryOperation.left, otherBinaryOperation.left) == Tristate::True
		    && equal(binaryOperation.right, otherBinaryOperation.right) == Tristate::True)
		{
			return Tristate::True;
		}

		// Only + and * are commutative operators, all others don’t need to be checked with exchanged arguments
		if (binaryOperation.operator_ != BinaryOperation::Operator::Plus
		    && binaryOperation.operator_ != BinaryOperation::Operator::Multiplication)
		{
			return Tristate::Unknown;
		}

		if (equal(binaryOperation.left, binaryOperation.right) == Tristate::True
		    && equal(binaryOperation.right, binaryOperation.left) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const Boolean &boolean, const Term &otherTerm)
	{
		if (!otherTerm.is<Boolean>())
			return Tristate::Unknown;

		const auto &otherBoolean = otherTerm.get<Boolean>();

		return (boolean.value == otherBoolean.value)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const Function &function, const Term &otherTerm)
	{
		if (!otherTerm.is<Function>())
			return Tristate::Unknown;

		const auto &otherFunction = otherTerm.get<Function>();

		if (function.declaration != otherFunction.declaration)
			return Tristate::False;

		if (function.arguments.size() != otherFunction.arguments.size())
			return Tristate::False;

		for (size_t i = 0; i < function.arguments.size(); i++)
			if (equal(function.arguments[i], otherFunction.arguments[i]) != Tristate::True)
				return Tristate::Unknown;

		return Tristate::True;
	}

	Tristate visit(const Integer &integer, const Term &otherTerm)
	{
		if (!otherTerm.is<Integer>())
			return Tristate::Unknown;

		const auto &otherInteger = otherTerm.get<Integer>();

		return (integer.value == otherInteger.value)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const Interval &interval, const Term &otherTerm)
	{
		if (!otherTerm.is<Interval>())
			return Tristate::Unknown;

		const auto &otherInterval = otherTerm.get<Interval>();

		if (equal(interval.from, otherInterval.from) != Tristate::True)
			return Tristate::Unknown;

		if (equal(interval.to, otherInterval.to) != Tristate::True)
			return Tristate::Unknown;

		return Tristate::True;
	}

	Tristate visit(const SpecialInteger &specialInteger, const Term &otherTerm)
	{
		if (!otherTerm.is<SpecialInteger>())
			return Tristate::Unknown;

		const auto &otherSpecialInteger = otherTerm.get<SpecialInteger>();

		return (specialInteger.type == otherSpecialInteger.type)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const String &string, const Term &otherTerm)
	{
		if (!otherTerm.is<String>())
			return Tristate::Unknown;

		const auto &otherString = otherTerm.get<String>();

		return (string.text == otherString.text)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const UnaryOperation &unaryOperation, const Term &otherTerm)
	{
		if (!otherTerm.is<UnaryOperation>())
			return Tristate::Unknown;

		const auto &otherUnaryOperation = otherTerm.get<UnaryOperation>();

		if (unaryOperation.operator_ != otherUnaryOperation.operator_)
			return Tristate::Unknown;

		return equal(unaryOperation.argument, otherUnaryOperation.argument);
	}

	Tristate visit(const Variable &variable, const Term &otherTerm)
	{
		if (!otherTerm.is<Variable>())
			return Tristate::Unknown;

		const auto &otherVariable = otherTerm.get<Variable>();

		return (variable.declaration == otherVariable.declaration)
			? Tristate::True
			: Tristate::False;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

Tristate equal(const Formula &lhs, const Formula &rhs)
{
	return lhs.accept(FormulaEqualityVisitor(), rhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Tristate equal(const Term &lhs, const Term &rhs)
{
	return lhs.accept(TermEqualityVisitor(), rhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
