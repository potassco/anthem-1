#ifndef __ANTHEM__EXAMINE_SEMANTICS__EQUALITY_H
#define __ANTHEM__EXAMINE_SEMANTICS__EQUALITY_H

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Utils.h>

namespace anthem
{
namespace examineSemantics
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Equality
//
////////////////////////////////////////////////////////////////////////////////////////////////////

Tristate equal(const ast::Formula &lhs, const ast::Formula &rhs);
Tristate equal(const ast::Term &lhs, const ast::Term &rhs);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormulaEqualityVisitor
{
	Tristate visit(const ast::And &and_, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::And>())
			return Tristate::Unknown;

		const auto &otherAnd = otherFormula.get<ast::And>();

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

	Tristate visit(const ast::Biconditional &biconditional, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Biconditional>())
			return Tristate::Unknown;

		const auto &otherBiconditional = otherFormula.get<ast::Biconditional>();

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

	Tristate visit(const ast::Boolean &boolean, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Boolean>())
			return Tristate::Unknown;

		const auto &otherBoolean = otherFormula.get<ast::Boolean>();

		return (boolean.value == otherBoolean.value)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const ast::Comparison &comparison, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Comparison>())
			return Tristate::Unknown;

		const auto &otherComparison = otherFormula.get<ast::Comparison>();

		if (comparison.operator_ != otherComparison.operator_)
			return Tristate::Unknown;

		if (equal(comparison.left, otherComparison.left) == Tristate::True
		    && equal(comparison.right, otherComparison.right) == Tristate::True)
		{
			return Tristate::True;
		}

		// Only = and != are commutative operators, all others don’t need to be checked with exchanged arguments
		if (comparison.operator_ != ast::Comparison::Operator::Equal
		    && comparison.operator_ != ast::Comparison::Operator::NotEqual)
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

	Tristate visit(const ast::Exists &, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Exists>())
			return Tristate::Unknown;

		// TODO: implement stronger check
		return Tristate::Unknown;
	}

	Tristate visit(const ast::ForAll &, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::ForAll>())
			return Tristate::Unknown;

		// TODO: implement stronger check
		return Tristate::Unknown;
	}

	Tristate visit(const ast::Implies &implies, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Implies>())
			return Tristate::Unknown;

		const auto &otherImplies = otherFormula.get<ast::Implies>();

		if (equal(implies.antecedent, otherImplies.antecedent) == Tristate::True
		    && equal(implies.consequent, otherImplies.consequent) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const ast::In &in, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::In>())
			return Tristate::Unknown;

		const auto &otherIn = otherFormula.get<ast::In>();

		if (equal(in.element, otherIn.element) == Tristate::True
		    && equal(in.set, otherIn.set) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const ast::Not &not_, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Not>())
			return Tristate::Unknown;

		const auto &otherNot = otherFormula.get<ast::Not>();

		return equal(not_.argument, otherNot.argument);
	}

	Tristate visit(const ast::Or &or_, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Or>())
			return Tristate::Unknown;

		const auto &otherOr = otherFormula.get<ast::Or>();

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

	Tristate visit(const ast::Predicate &predicate, const ast::Formula &otherFormula)
	{
		if (!otherFormula.is<ast::Predicate>())
			return Tristate::Unknown;

		const auto &otherPredicate = otherFormula.get<ast::Predicate>();

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
	Tristate visit(const ast::BinaryOperation &binaryOperation, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::BinaryOperation>())
			return Tristate::Unknown;

		const auto &otherBinaryOperation = otherTerm.get<ast::BinaryOperation>();

		if (binaryOperation.operator_ != otherBinaryOperation.operator_)
			return Tristate::Unknown;

		if (equal(binaryOperation.left, otherBinaryOperation.left) == Tristate::True
		    && equal(binaryOperation.right, otherBinaryOperation.right) == Tristate::True)
		{
			return Tristate::True;
		}

		// Only + and * are commutative operators, all others don’t need to be checked with exchanged arguments
		if (binaryOperation.operator_ != ast::BinaryOperation::Operator::Plus
		    && binaryOperation.operator_ != ast::BinaryOperation::Operator::Multiplication)
		{
			return Tristate::Unknown;
		}

		if (equal(binaryOperation.left, otherBinaryOperation.right) == Tristate::True
		    && equal(binaryOperation.right, otherBinaryOperation.left) == Tristate::True)
		{
			return Tristate::True;
		}

		return Tristate::Unknown;
	}

	Tristate visit(const ast::Boolean &boolean, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::Boolean>())
			return Tristate::Unknown;

		const auto &otherBoolean = otherTerm.get<ast::Boolean>();

		return (boolean.value == otherBoolean.value)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const ast::Function &function, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::Function>())
			return Tristate::Unknown;

		const auto &otherFunction = otherTerm.get<ast::Function>();

		if (function.declaration != otherFunction.declaration)
			return Tristate::False;

		if (function.arguments.size() != otherFunction.arguments.size())
			return Tristate::False;

		for (size_t i = 0; i < function.arguments.size(); i++)
			if (equal(function.arguments[i], otherFunction.arguments[i]) != Tristate::True)
				return Tristate::Unknown;

		return Tristate::True;
	}

	Tristate visit(const ast::Integer &integer, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::Integer>())
			return Tristate::Unknown;

		const auto &otherInteger = otherTerm.get<ast::Integer>();

		return (integer.value == otherInteger.value)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const ast::Interval &interval, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::Interval>())
			return Tristate::Unknown;

		const auto &otherInterval = otherTerm.get<ast::Interval>();

		if (equal(interval.from, otherInterval.from) != Tristate::True)
			return Tristate::Unknown;

		if (equal(interval.to, otherInterval.to) != Tristate::True)
			return Tristate::Unknown;

		return Tristate::True;
	}

	Tristate visit(const ast::SpecialInteger &specialInteger, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::SpecialInteger>())
			return Tristate::Unknown;

		const auto &otherSpecialInteger = otherTerm.get<ast::SpecialInteger>();

		return (specialInteger.type == otherSpecialInteger.type)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const ast::String &string, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::String>())
			return Tristate::Unknown;

		const auto &otherString = otherTerm.get<ast::String>();

		return (string.text == otherString.text)
			? Tristate::True
			: Tristate::False;
	}

	Tristate visit(const ast::UnaryOperation &unaryOperation, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::UnaryOperation>())
			return Tristate::Unknown;

		const auto &otherUnaryOperation = otherTerm.get<ast::UnaryOperation>();

		if (unaryOperation.operator_ != otherUnaryOperation.operator_)
			return Tristate::Unknown;

		return equal(unaryOperation.argument, otherUnaryOperation.argument);
	}

	Tristate visit(const ast::Variable &variable, const ast::Term &otherTerm)
	{
		if (!otherTerm.is<ast::Variable>())
			return Tristate::Unknown;

		const auto &otherVariable = otherTerm.get<ast::Variable>();

		return (variable.declaration == otherVariable.declaration)
			? Tristate::True
			: Tristate::False;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline Tristate equal(const ast::Formula &lhs, const ast::Formula &rhs)
{
	return lhs.accept(FormulaEqualityVisitor(), rhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline Tristate equal(const ast::Term &lhs, const ast::Term &rhs)
{
	return lhs.accept(TermEqualityVisitor(), rhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
