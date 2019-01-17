#ifndef __ANTHEM__TYPE_H
#define __ANTHEM__TYPE_H

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Type
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor = DefaultVariableDomainAccessor, class... Arguments>
Type type(const ast::Term &term, Arguments &&... arguments);

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor = DefaultVariableDomainAccessor>
struct TermTypeVisitor
{
	template <class... Arguments>
	static Type visit(const ast::BinaryOperation &binaryOperation, Arguments &&... arguments)
	{
		const auto leftType = type<VariableDomainAccessor>(binaryOperation.left, std::forward<Arguments>(arguments)...);
		const auto rightType = type<VariableDomainAccessor>(binaryOperation.right, std::forward<Arguments>(arguments)...);

		// Binary operations on empty sets return an empty set (also with division)
		if (leftType.setSize == SetSize::Empty || rightType.setSize == SetSize::Empty)
			return {Domain::Unknown, SetSize::Empty};

		// Binary operations on symbolic variables return an empty set (also with division)
		if (leftType.domain == Domain::Symbolic || rightType.domain == Domain::Symbolic)
			return {Domain::Unknown, SetSize::Empty};

		// Binary operations on unknown types return an unknown set
		if (leftType.domain == Domain::Unknown || rightType.domain == Domain::Unknown)
			return {Domain::Unknown, SetSize::Unknown};

		// Divisions return an unknown set
		if (binaryOperation.operator_ == ast::BinaryOperation::Operator::Division)
			return {Domain::Integer, SetSize::Unknown};

		// Binary operations on integer sets of unknown size return an integer set of unknown size
		if (leftType.setSize == SetSize::Unknown || rightType.setSize == SetSize::Unknown)
			return {Domain::Integer, SetSize::Unknown};

		// Binary operations on integer sets with multiple elements return an integer set with multiple elements
		if (leftType.setSize == SetSize::Multi || rightType.setSize == SetSize::Multi)
			return {Domain::Integer, SetSize::Multi};

		// Binary operations on plain integers return a plain integer
		return {Domain::Integer, SetSize::Unit};
	}

	template <class... Arguments>
	static Type visit(const ast::Boolean &, Arguments &&...)
	{
		return {Domain::Symbolic, SetSize::Unit};
	}

	template <class... Arguments>
	static Type visit(const ast::Function &function, Arguments &&...)
	{
		// TODO: check that functions cannot return sets

		return {function.declaration->domain, SetSize::Unit};
	}

	template <class... Arguments>
	static Type visit(const ast::Integer &, Arguments &&...)
	{
		return {Domain::Integer, SetSize::Unit};
	}

	template <class... Arguments>
	static Type visit(const ast::Interval &interval, Arguments &&... arguments)
	{
		const auto fromType = type<VariableDomainAccessor>(interval.from, std::forward<Arguments>(arguments)...);
		const auto toType = type<VariableDomainAccessor>(interval.to, std::forward<Arguments>(arguments)...);

		// Intervals with empty sets return an empty set
		if (fromType.setSize == SetSize::Empty || toType.setSize == SetSize::Empty)
			return {Domain::Unknown, SetSize::Empty};

		// Intervals with symbolic variables return an empty set
		if (fromType.domain == Domain::Symbolic || toType.domain == Domain::Symbolic)
			return {Domain::Unknown, SetSize::Empty};

		// Intervals with unknown types return an unknown set
		if (fromType.domain == Domain::Unknown || toType.domain == Domain::Unknown)
			return {Domain::Unknown, SetSize::Unknown};

		// Intervals with integers generally return integer sets
		// TODO: handle 1-element intervals such as 1..1 and empty intervals such as 2..1
		return {Domain::Integer, SetSize::Unknown};
	}

	template <class... Arguments>
	static Type visit(const ast::SpecialInteger &, Arguments &&...)
	{
		return {Domain::Symbolic, SetSize::Unit};
	}

	template <class... Arguments>
	static Type visit(const ast::String &, Arguments &&...)
	{
		return {Domain::Symbolic, SetSize::Unit};
	}

	template <class... Arguments>
	static Type visit(const ast::UnaryOperation &unaryOperation, Arguments &&... arguments)
	{
		assert(unaryOperation.operator_ == ast::UnaryOperation::Operator::Absolute
			|| unaryOperation.operator_ == ast::UnaryOperation::Operator::Minus);

		const auto argumentType = type<VariableDomainAccessor>(unaryOperation.argument, std::forward<Arguments>(arguments)...);

		// Absolute/negative value of an empty set returns an empty set
		if (argumentType.setSize == SetSize::Empty)
			return {Domain::Unknown, SetSize::Empty};

		// Absolute/negative value of symbolic variables returns an empty set
		if (argumentType.domain == Domain::Symbolic)
			return {Domain::Unknown, SetSize::Empty};

		// Absolute/negative value of integers returns the same type
		if (argumentType.domain == Domain::Integer)
			return argumentType;

		return {Domain::Unknown, SetSize::Unknown};
	}

	template <class... Arguments>
	static Type visit(const ast::Variable &variable, Arguments &&... arguments)
	{
		const auto domain = VariableDomainAccessor()(variable, std::forward<Arguments>(arguments)...);

		return {domain, SetSize::Unit};
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor, class... Arguments>
Type type(const ast::Term &term, Arguments &&... arguments)
{
	return term.accept(TermTypeVisitor<VariableDomainAccessor>(), std::forward<Arguments>(arguments)...);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
