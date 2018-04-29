#ifndef __ANTHEM__EVALUATION_H
#define __ANTHEM__EVALUATION_H

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Evaluation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor = DefaultVariableDomainAccessor>
struct EvaluateFormulaVisitor
{
	template <class... Arguments>
	static EvaluationResult visit(const ast::And &and_, Arguments &&... arguments)
	{
		bool someFalse = false;
		bool someUnknown = false;

		for (const auto &argument : and_.arguments)
		{
			const auto result = evaluate(argument, std::forward<Arguments>(arguments)...);

			switch (result)
			{
				case EvaluationResult::Error:
					return EvaluationResult::Error;
				case EvaluationResult::True:
					break;
				case EvaluationResult::False:
					someFalse = true;
					break;
				case EvaluationResult::Unknown:
					someUnknown = true;
					break;
			}
		}

		if (someFalse)
			return EvaluationResult::False;

		if (someUnknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::True;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Biconditional &biconditional, Arguments &&... arguments)
	{
		const auto leftResult = evaluate(biconditional.left, std::forward<Arguments>(arguments)...);
		const auto rightResult = evaluate(biconditional.right, std::forward<Arguments>(arguments)...);

		if (leftResult == EvaluationResult::Error || rightResult == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (leftResult == EvaluationResult::Unknown || rightResult == EvaluationResult::Unknown)
			return EvaluationResult::Unknown;

		return (leftResult == rightResult ? EvaluationResult::True : EvaluationResult::False);
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Boolean &boolean, Arguments &&...)
	{
		return (boolean.value == true ? EvaluationResult::True : EvaluationResult::False);
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Comparison &comparison, Arguments &&... arguments)
	{
		const auto leftType = type(comparison.left, std::forward<Arguments>(arguments)...);
		const auto rightType = type(comparison.right, std::forward<Arguments>(arguments)...);

		// Comparisons with empty sets always return false
		if (leftType.setSize == SetSize::Empty || rightType.setSize == SetSize::Empty)
			return EvaluationResult::False;

		// If either side has an unknown domain, the result is unknown
		if (leftType.domain == Domain::Unknown || rightType.domain == Domain::Unknown)
			return EvaluationResult::Unknown;

		// If both sides have the same domain, the result is unknown
		if (leftType.domain == rightType.domain)
			return EvaluationResult::Unknown;

		// If one side is integer, but the other one isn’t, they are not equal
		switch (comparison.operator_)
		{
			case ast::Comparison::Operator::Equal:
				return EvaluationResult::False;
			case ast::Comparison::Operator::NotEqual:
				return EvaluationResult::True;
			default:
				// TODO: implement more cases
				return EvaluationResult::Unknown;
		}
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Exists &exists, Arguments &&... arguments)
	{
		return evaluate(exists.argument, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::ForAll &forAll, Arguments &&... arguments)
	{
		return evaluate(forAll.argument, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Implies &implies, Arguments &&... arguments)
	{
		const auto antecedentResult = evaluate(implies.antecedent, std::forward<Arguments>(arguments)...);
		const auto consequentResult = evaluate(implies.consequent, std::forward<Arguments>(arguments)...);

		if (antecedentResult == EvaluationResult::Error || consequentResult == EvaluationResult::Error)
			return EvaluationResult::Error;

		if (antecedentResult == EvaluationResult::False)
			return EvaluationResult::True;

		if (consequentResult == EvaluationResult::True)
			return EvaluationResult::True;

		if (antecedentResult == EvaluationResult::True && consequentResult == EvaluationResult::False)
			return EvaluationResult::False;

		return EvaluationResult::Unknown;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::In &in, Arguments &&... arguments)
	{
		const auto elementType = type(in.element, std::forward<Arguments>(arguments)...);
		const auto setType = type(in.set, std::forward<Arguments>(arguments)...);

		// The element to test shouldn’t be empty or a proper set by itself
		assert(elementType.setSize != SetSize::Empty && elementType.setSize != SetSize::Multi);

		// If the set is empty, no element can be selected
		if (setType.setSize == SetSize::Empty)
			return EvaluationResult::False;

		// If one of the sides has an unknown type, the result is unknown
		if (elementType.domain == Domain::Unknown || setType.domain == Domain::Unknown)
			return EvaluationResult::Unknown;

		// If both sides have the same domain, the result is unknown
		if (elementType.domain == setType.domain)
			return EvaluationResult::Unknown;

		// If one side is integer, but the other one isn’t, set inclusion is never satisfied
		return EvaluationResult::False;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Not &not_, Arguments &&... arguments)
	{
		const auto result = evaluate(not_.argument, std::forward<Arguments>(arguments)...);

		if (result == EvaluationResult::Error || result == EvaluationResult::Unknown)
			return result;

		return (result == EvaluationResult::True ? EvaluationResult::False : EvaluationResult::True);
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Or &or_, Arguments &&... arguments)
	{
		bool someTrue = false;
		bool someUnknown = false;

		for (const auto &argument : or_.arguments)
		{
			const auto result = evaluate(argument, std::forward<Arguments>(arguments)...);

			switch (result)
			{
				case EvaluationResult::Error:
					return EvaluationResult::Error;
				case EvaluationResult::True:
					someTrue = true;
					break;
				case EvaluationResult::False:
					break;
				case EvaluationResult::Unknown:
					someUnknown = true;
					break;
			}
		}

		if (someTrue)
			return EvaluationResult::True;

		if (someUnknown)
			return EvaluationResult::Unknown;

		return EvaluationResult::False;
	}

	template <class... Arguments>
	static EvaluationResult visit(const ast::Predicate &predicate, Arguments &&... arguments)
	{
		assert(predicate.arguments.size() == predicate.declaration->arity());

		for (size_t i = 0; i < predicate.arguments.size(); i++)
		{
			const auto &argument = predicate.arguments[i];
			const auto &parameter = predicate.declaration->parameters[i];

			if (parameter.domain != Domain::Integer)
				continue;

			const auto argumentType = type(argument, std::forward<Arguments>(arguments)...);

			if (argumentType.domain == Domain::Noninteger || argumentType.setSize == SetSize::Empty)
				return EvaluationResult::Error;
		}

		return EvaluationResult::Unknown;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class VariableDomainAccessor = DefaultVariableDomainAccessor, class... Arguments>
EvaluationResult evaluate(const ast::Formula &formula, Arguments &&... arguments)
{
	return formula.accept(EvaluateFormulaVisitor<VariableDomainAccessor>(), std::forward<Arguments>(arguments)...);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
