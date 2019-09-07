#ifndef __ANTHEM__OUTPUT__FORMATTER_TPTP_H
#define __ANTHEM__OUTPUT__FORMATTER_TPTP_H

#include <cassert>

#include <anthem/AST.h>
#include <anthem/Exception.h>
#include <anthem/Utils.h>
#include <anthem/output/ColorStream.h>
#include <anthem/output/Formatter.h>
#include <anthem/output/Formatting.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FormatterTPTP
//
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const auto TPTPTypeHeader =
	R"(%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% types
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(types, type, object: $tType).
)";

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const auto TPTPPreamble =
	R"(
tff(types, type, (f__integer__: ($int) > object)).
tff(types, type, (f__symbolic__: ($i) > object)).
tff(types, type, (c__infimum__: object)).
tff(types, type, (c__supremum__: object)).

tff(types, type, (f__sum__: (object * object) > object)).
tff(types, type, (f__unary_minus__: (object) > object)).
tff(types, type, (f__difference__: (object * object) > object)).
tff(types, type, (f__product__: (object * object) > object)).

tff(types, type, (p__is_integer__: (object) > $o)).
tff(types, type, (p__is_symbolic__: (object) > $o)).
tff(types, type, (p__less_equal__: (object * object) > $o)).
tff(types, type, (p__less__: (object * object) > $o)).
tff(types, type, (p__greater_equal__: (object * object) > $o)).
tff(types, type, (p__greater__: (object * object) > $o)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% objects: integers vs. symbolics
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(types, axiom, (![X: object]: (p__is_integer__(X) <=> (?[Y: $int]: (X = f__integer__(Y)))))).
tff(types, axiom, (![X: object]: (p__is_symbolic__(X) <=> (?[Y: $i]: (X = f__symbolic__(Y)))))).
tff(types, axiom, (![X: object]: ((X = c__infimum__) | p__is_integer__(X) | p__is_symbolic__(X) | (X = c__supremum__)))).
tff(types, axiom, (![X: $int, Y: $int]: ((f__integer__(X) = f__integer__(Y)) <=> (X = Y)))).
tff(types, axiom, (![X: $i, Y: $i]: ((f__symbolic__(X) = f__symbolic__(Y)) <=> (X = Y)))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% integer operations
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(operations, axiom, (![X1: $int, X2: $int]: (f__sum__(f__integer__(X1), f__integer__(X2)) = f__integer__($sum(X1, X2))))).
tff(operations, axiom, (![X: $int]: (f__unary_minus__(f__integer__(X)) = f__integer__($uminus(X))))).
tff(operations, axiom, (![X1: $int, X2: $int]: (f__difference__(f__integer__(X1), f__integer__(X2)) = f__integer__($difference(X1, X2))))).
tff(operations, axiom, (![X1: $int, X2: $int]: (f__product__(f__integer__(X1), f__integer__(X2)) = f__integer__($product(X1, X2))))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% object comparisons
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(less_equal, axiom, (![X1: $int, X2: $int]: (p__less_equal__(f__integer__(X1), f__integer__(X2)) <=> $lesseq(X1, X2)))).
tff(less_equal, axiom, (![X1: object, X2: object]: ((p__less_equal__(X1, X2) & p__less_equal__(X2, X1)) => (X1 = X2)))).
tff(less_equal, axiom, (![X1: object, X2: object, X3: object]: ((p__less_equal__(X1, X2) & p__less_equal__(X2, X3)) => p__less_equal__(X1, X3)))).
tff(less_equal, axiom, (![X1: object, X2: object]: (p__less_equal__(X1, X2) | p__less_equal__(X2, X1)))).

tff(less, axiom, (![X1: object, X2: object]: (p__less__(X1, X2) <=> (p__less_equal__(X1, X2) & (X1 != X2))))).
tff(greater_equal, axiom, (![X1: object, X2: object]: (p__greater_equal__(X1, X2) <=> p__less_equal__(X2, X1)))).
tff(greater, axiom, (![X1: object, X2: object]: (p__greater__(X1, X2) <=> (p__less_equal__(X2, X1) & (X1 != X2))))).

tff(type_order, axiom, (![X: $int]: p__less__(c__infimum__, f__integer__(X)))).
tff(type_order, axiom, (![X1: $int, X2: $i]: p__less__(f__integer__(X1), f__symbolic__(X2)))).
tff(type_order, axiom, (![X: $i]: p__less__(f__symbolic__(X), c__supremum__))).
)";

////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormatterTPTP
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Primitives
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::BinaryOperation::Operator operator_, PrintContext &, bool)
	{
		switch (operator_)
		{
			case ast::BinaryOperation::Operator::Plus:
				return (stream << output::Keyword(AuxiliaryFunctionNameSum));
			case ast::BinaryOperation::Operator::Minus:
				return (stream << output::Keyword(AuxiliaryFunctionNameDifference));
			case ast::BinaryOperation::Operator::Multiplication:
				return (stream << output::Keyword(AuxiliaryFunctionNameProduct));
			case ast::BinaryOperation::Operator::Division:
				throw LogicException("division operator not expected for TPTP, please report to bug tracker");
			case ast::BinaryOperation::Operator::Modulo:
				throw LogicException("modulo operator not expected for TPTP, please report to bug tracker");
			case ast::BinaryOperation::Operator::Power:
				throw LogicException("power operator not expected for TPTP, please report to bug tracker");
			default:
				throw LogicException("binary operator not expected for TPTP, please report to bug tracker");
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::BinaryOperation &binaryOperation, PrintContext &printContext, bool)
	{
		print(stream, binaryOperation.operator_, printContext, true);
		stream << "(";
		print(stream, binaryOperation.left, printContext, false);
		stream << ", ";
		print(stream, binaryOperation.right, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Boolean &boolean, PrintContext &, bool)
	{
		if (boolean.value)
			return (stream << output::Boolean("$true"));

		return (stream << output::Boolean("$false"));
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Comparison &comparison, PrintContext &printContext, bool)
	{
		if (comparison.operator_ == ast::Comparison::Operator::Equal || comparison.operator_ == ast::Comparison::Operator::NotEqual)
		{
			stream << "(";
			print(stream, comparison.left, printContext, false);

			if (comparison.operator_ == ast::Comparison::Operator::Equal)
				stream << " = ";
			else if (comparison.operator_ == ast::Comparison::Operator::NotEqual)
				stream << " != ";

			print(stream, comparison.right, printContext, false);
			stream << ")";

			return stream;
		}

		switch (comparison.operator_)
		{
			// TODO: rename and reorder for consistency
			case ast::Comparison::Operator::GreaterThan:
				stream << output::Keyword(AuxiliaryPredicateNameGreater);
				break;
			case ast::Comparison::Operator::LessThan:
				stream << output::Keyword(AuxiliaryPredicateNameLess);
				break;
			case ast::Comparison::Operator::LessEqual:
				stream << output::Keyword(AuxiliaryPredicateNameLessEqual);
				break;
			case ast::Comparison::Operator::GreaterEqual:
				stream << output::Keyword(AuxiliaryPredicateNameGreaterEqual);
				break;
			default:
				throw LogicException("unexpected comparison operator, please report to bug tracker");
		}

		stream << "(";
		print(stream, comparison.left, printContext, false);
		stream << ", ";
		print(stream, comparison.right, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Function &function, PrintContext &printContext, bool)
	{
		stream << function.declaration->name;

		if (function.arguments.empty())
			return stream;

		stream << "(";

		for (auto i = function.arguments.cbegin(); i != function.arguments.cend(); i++)
		{
			if (i != function.arguments.cbegin())
				stream << ", ";

			print(stream, *i, printContext, true);
		}

		if (function.declaration->name.empty() && function.arguments.size() == 1)
			stream << ",";

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::FunctionDeclaration &functionDeclaration, PrintContext &, bool)
	{
		return (stream << functionDeclaration.name << "/" << functionDeclaration.arity());
	}

	static output::ColorStream &print(output::ColorStream &, const ast::In &, PrintContext &, bool)
	{
		throw LogicException("set inclusion operator not expected for TPTP, please report to bug tracker");
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Integer &integer, PrintContext &, bool)
	{
		if (integer.value < 0)
			return (stream << output::Keyword("$uminus") << "(" << output::Number<int>(-integer.value) << ")");

		return (stream << output::Number<int>(integer.value));
	}

	static output::ColorStream &print(output::ColorStream &, const ast::Interval &, PrintContext &, bool)
	{
		throw LogicException("intervals not expected for TPTP, please report to bug tracker");
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Predicate &predicate, PrintContext &printContext, bool)
	{
		stream << predicate.declaration->name;

		if (predicate.arguments.empty())
			return stream;

		stream << "(";

		for (auto i = predicate.arguments.cbegin(); i != predicate.arguments.cend(); i++)
		{
			if (i != predicate.arguments.cbegin())
				stream << ", ";

			print(stream, *i, printContext, false);
		}

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::PredicateDeclaration &predicateDeclaration, PrintContext &, bool)
	{
		return (stream << predicateDeclaration.name << "/" << predicateDeclaration.arity());
	}

	static output::ColorStream &print(output::ColorStream &, const ast::SpecialInteger &, PrintContext &, bool)
	{
		throw TranslationException("special integers not implemented for TPTP");
	}

	static output::ColorStream &print(output::ColorStream &, const ast::String &, PrintContext &, bool)
	{
		throw TranslationException("strings not implemented for TPTP");
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::UnaryOperation &unaryOperation, PrintContext &printContext, bool)
	{
		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				throw TranslationException("absolute value not implemented for TPTP");
			case ast::UnaryOperation::Operator::Minus:
				stream << output::Keyword(AuxiliaryFunctionNameUnaryMinus) << "(";
				break;
			default:
				throw LogicException("unexpected unary operator, please report to bug tracker");
		}

		print(stream, unaryOperation.argument, printContext, true);

		switch (unaryOperation.operator_)
		{
			case ast::UnaryOperation::Operator::Absolute:
				throw TranslationException("absolute value not implemented for TPTP");
			case ast::UnaryOperation::Operator::Minus:
				stream << ")";
				break;
			default:
				throw LogicException("unexpected unary operator, please report to bug tracker");
		}

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Variable &variable, PrintContext &printContext, bool)
	{
		assert(variable.declaration != nullptr);

		return print(stream, *variable.declaration, printContext, true);
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::VariableDeclaration &variableDeclaration, PrintContext &printContext, bool)
	{
		const auto printVariableDeclaration =
			[&stream, &variableDeclaration](const auto *prefix, auto &variableIDs) -> output::ColorStream &
			{
				auto matchingVariableID = variableIDs.find(&variableDeclaration);

				if (matchingVariableID == variableIDs.cend())
				{
					auto emplaceResult = variableIDs.emplace(std::make_pair(&variableDeclaration, variableIDs.size() + 1));
					assert(emplaceResult.second);
					matchingVariableID = emplaceResult.first;
				}

				const auto variableName = std::string(prefix) + std::to_string(matchingVariableID->second);

				return (stream << output::Variable(variableName.c_str()));
			};

		if (variableDeclaration.domain != Domain::Union)
			throw LogicException("expected all variables to have union type, please report to bug tracker");

		switch (variableDeclaration.type)
		{
			case ast::VariableDeclaration::Type::UserDefined:
				printVariableDeclaration(UserVariablePrefix, printContext.userVariableIDs);
				break;
			default:
				printVariableDeclaration(BodyVariablePrefix, printContext.bodyVariableIDs);
				break;
		}

		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Expressions
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::And &and_, PrintContext &printContext, bool)
	{
		stream << "(";

		for (auto i = and_.arguments.cbegin(); i != and_.arguments.cend(); i++)
		{
			if (i != and_.arguments.cbegin())
				stream << " " << output::Operator("&") << " ";

			print(stream, *i, printContext, false);
		}

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Biconditional &biconditional, PrintContext &printContext, bool)
	{
		stream << "(";

		print(stream, biconditional.left, printContext, false);
		stream << " <=> ";
		print(stream, biconditional.right, printContext, false);

		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Exists &exists, PrintContext &printContext, bool)
	{
		stream << "(" << output::Operator("?") << "[";

		for (auto i = exists.variables.cbegin(); i != exists.variables.cend(); i++)
		{
			const auto &variableDeclaration = **i;

			if (i != exists.variables.cbegin())
				stream << ", ";

			print(stream, variableDeclaration, printContext, true);
			stream << ": " << output::Keyword("object");
		}

		stream << "]: ";
		print(stream, exists.argument, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::ForAll &forAll, PrintContext &printContext, bool)
	{
		stream << "(" << output::Operator("!") << "[";

		for (auto i = forAll.variables.cbegin(); i != forAll.variables.cend(); i++)
		{
			const auto &variableDeclaration = **i;

			if (i != forAll.variables.cbegin())
				stream << ", ";

			print(stream, variableDeclaration, printContext, true);
			stream << ": " << output::Keyword("object");
		}

		stream << "]: ";
		print(stream, forAll.argument, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Implies &implies, PrintContext &printContext, bool)
	{
		stream << "(";
		print(stream, implies.antecedent, printContext, false);
		stream << " => ";
		print(stream, implies.consequent, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Not &not_, PrintContext &printContext, bool)
	{
		stream << "(" << output::Operator("~");
		print(stream, not_.argument, printContext, false);
		stream << ")";

		return stream;
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Or &or_, PrintContext &printContext, bool omitParentheses)
	{
		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << "(";

		for (auto i = or_.arguments.cbegin(); i != or_.arguments.cend(); i++)
		{
			if (i != or_.arguments.cbegin())
				stream << " " << output::Operator("|") << " ";

			print(stream, *i, printContext, true);
		}

		if (!omitParentheses || printContext.context.parenthesisStyle == ParenthesisStyle::Full)
			stream << ")";

		return stream;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Variants
	////////////////////////////////////////////////////////////////////////////////////////////////

	static output::ColorStream &print(output::ColorStream &stream, const ast::Formula &formula, PrintContext &printContext, bool omitParentheses)
	{
		return formula.accept(VariantPrintVisitor<FormatterTPTP, ast::Formula>(), stream, printContext, omitParentheses);
	}

	static output::ColorStream &print(output::ColorStream &stream, const ast::Term &term, PrintContext &printContext, bool omitParentheses)
	{
		return term.accept(VariantPrintVisitor<FormatterTPTP, ast::Term>(), stream, printContext, omitParentheses);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
