#ifndef __ANTHEM__ARITHMETICS_H
#define __ANTHEM__ARITHMETICS_H

#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Arithmetics
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct DefaultVariableDomainAccessor
{
	ast::Domain operator()(const ast::Variable &variable)
	{
		return variable.declaration->domain;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor = DefaultVariableDomainAccessor, class... Arguments>
EvaluationResult isArithmetic(const ast::Term &term, Arguments &&...);

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor = DefaultVariableDomainAccessor>
struct IsTermArithmeticVisitor
{
	template <class... Arguments>
	static EvaluationResult visit(const ast::BinaryOperation &binaryOperation, Arguments &&... arguments)
	{
		const auto isLeftArithemtic = isArithmetic<VariableDomainAccessor>(binaryOperation.left, std::forward<Arguments>(arguments)...);
		const auto isRightArithmetic = isArithmetic<VariableDomainAccessor>(binaryOperation.right, std::forward<Arguments>(arguments)...);

		if (isLeftArithemtic == EvaluationResult::Error || isRightArithmetic == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (isLeftArithemtic == EvaluationResult::False || isRightArithmetic == EvaluationResult::False)
			return EvaluationResult::Error;

		if (isLeftArithemtic == EvaluationResult::Unknown || isRightArithmetic == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::True;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Boolean &, Arguments &&...)
	{
		return EvaluationResult::False;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Function &function, Arguments &&...)
	{
		switch (function.declaration->domain)
		{
			case ast::Domain::General:
				return EvaluationResult::False;
			case ast::Domain::Integer:
				return EvaluationResult::True;
			case ast::Domain::Unknown:
				return EvaluationResult::Unknown;
		}

		return EvaluationResult::Unknown;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Integer &, Arguments &&...)
	{
		return EvaluationResult::True;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Interval &interval, Arguments &&... arguments)
	{
		const auto isFromArithmetic = isArithmetic<VariableDomainAccessor>(interval.from, std::forward<Arguments>(arguments)...);
		const auto isToArithmetic = isArithmetic<VariableDomainAccessor>(interval.to, std::forward<Arguments>(arguments)...);

		if (isFromArithmetic == EvaluationResult::Error || isToArithmetic == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (isFromArithmetic == EvaluationResult::False || isToArithmetic == EvaluationResult::False)
			return EvaluationResult::Error;

		if (isFromArithmetic == EvaluationResult::Unknown || isToArithmetic == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::True;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::SpecialInteger &, Arguments &&...)
	{
		return EvaluationResult::False;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::String &, Arguments &&...)
	{
		return EvaluationResult::False;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::UnaryOperation &unaryOperation, Arguments &&... arguments)
	{
		const auto isArgumentArithmetic = isArithmetic<VariableDomainAccessor>(unaryOperation.argument, std::forward<Arguments>(arguments)...);

		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				return (isArgumentArithmetic == EvaluationResult::False ? EvaluationResult::Error : isArgumentArithmetic);
		}

		return EvaluationResult::Unknown;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Variable &variable, Arguments &&... arguments)
	{
		const auto domain = VariableDomainAccessor()(variable, std::forward<Arguments>(arguments)...);

		switch (domain)
		{
			case ast::Domain::General:
				return EvaluationResult::False;
			case ast::Domain::Integer:
				return EvaluationResult::True;
			case ast::Domain::Unknown:
				return EvaluationResult::Unknown;
		}

		return EvaluationResult::Unknown;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor, class... Arguments>
EvaluationResult isArithmetic(const ast::Term &term, Arguments &&... arguments)
{
	return term.accept(IsTermArithmeticVisitor<VariableDomainAccessor>(), std::forward<Arguments>(arguments)...);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
