#include <anthem/translation-common/UnifyDomains.h>

#include <optional>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Exception.h>
#include <anthem/Utils.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// UnifyDomains
//
////////////////////////////////////////////////////////////////////////////////////////////////////

Domain unifyDomains(ast::Term &term, Context &context);

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::FunctionDeclaration *findOrCreateAuxiliaryUnionFunctionDeclaration(const char *functionName,
	size_t arity, Context &context)
{
	auto auxiliaryUnionFunctionDeclaration
		= context.findOrCreateFunctionDeclaration(functionName, arity);

	auxiliaryUnionFunctionDeclaration->domain = Domain::Union;

	return auxiliaryUnionFunctionDeclaration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormulaUnifyDomainsVisitor
{
	void visit(ast::And &and_, ast::Formula &, Context &context)
	{
		for (auto &argument : and_.arguments)
			unifyDomains(argument, context);
	}

	void visit(ast::Biconditional &biconditional, ast::Formula &, Context &context)
	{
		unifyDomains(biconditional.left, context);
		unifyDomains(biconditional.right, context);
	}

	void visit(ast::Boolean &, ast::Formula &, Context &)
	{
	}

	void visit(ast::Comparison &comparison, ast::Formula &formula, Context &context)
	{
		const auto domainLeft = unifyDomains(comparison.left, context);
		const auto domainRight = unifyDomains(comparison.right, context);

		const auto auxiliaryPredicateNameComparison =
			[&]()
			{
				switch (comparison.operator_)
				{
					case ast::Comparison::Operator::GreaterThan:
						return AuxiliaryPredicateNameGreater;
					case ast::Comparison::Operator::LessThan:
						return AuxiliaryPredicateNameLess;
					case ast::Comparison::Operator::LessEqual:
						return AuxiliaryPredicateNameLessEqual;
					case ast::Comparison::Operator::GreaterEqual:
						return AuxiliaryPredicateNameGreaterEqual;
					default:
						throw LogicException("unexpected comparison operator, please report to bug tracker");
				}
			};

		const auto auxiliaryFunctionDeclarationInteger
			= findOrCreateAuxiliaryUnionFunctionDeclaration(AuxiliaryFunctionNameInteger, 1, context);

		// Check that both sides are of union or integer type
		if ((domainLeft != Domain::Union && domainLeft != Domain::Integer)
			|| (domainRight != Domain::Union && domainRight != Domain::Integer))
		{
			throw LogicException("unexpected type of comparison operand, please report to bug tracker");
		}

		// If both sides are of integer type, return
		if (domainLeft == Domain::Integer && domainRight == Domain::Integer)
			return;

		// If the left side is integer, map it to a union-type object
		if (domainLeft == Domain::Integer)
		{
			std::vector<ast::Term> arguments;
			arguments.reserve(1);
			arguments.emplace_back(std::move(comparison.left));
			comparison.left = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
		}

		// If the right side is integer, map it to a union-type object
		if (domainRight == Domain::Integer)
		{
			std::vector<ast::Term> arguments;
			arguments.reserve(1);
			arguments.emplace_back(std::move(comparison.right));
			comparison.right = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
		}

		if (comparison.operator_ == ast::Comparison::Operator::Equal
			|| comparison.operator_ == ast::Comparison::Operator::NotEqual)
		{
			return;
		}

		const auto auxiliaryPredicateDeclarationComparison
			= context.findOrCreatePredicateDeclaration(auxiliaryPredicateNameComparison(), 2);

		std::vector<ast::Term> arguments;
		arguments.reserve(2);
		arguments.emplace_back(std::move(comparison.left));
		arguments.emplace_back(std::move(comparison.right));
		formula = ast::Predicate(auxiliaryPredicateDeclarationComparison, std::move(arguments));
	}

	void visit(ast::Exists &exists, ast::Formula &, Context &context)
	{
		unifyDomains(exists.argument, context);

		if (exists.variables.empty())
			return;

		ast::And and_;

		const auto auxiliaryPredicateDeclarationIsInteger =
			context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameIsInteger, 1);

		for (auto &variableDeclaration : exists.variables)
		{
			switch (variableDeclaration->domain)
			{
				case Domain::Integer:
				{
					auto predicate = ast::Predicate(auxiliaryPredicateDeclarationIsInteger);
					predicate.arguments.reserve(1);
					predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
					and_.arguments.emplace_back(std::move(predicate));

					break;
				}
				case Domain::Program:
					variableDeclaration->domain = Domain::Union;
					break;
				default:
					throw LogicException("unexpected parameter domain, please report to bug tracker");
			}
		}
	}

	void visit(ast::ForAll &forAll, ast::Formula &, Context &context)
	{
		unifyDomains(forAll.argument, context);

		if (forAll.variables.empty())
			return;

		ast::And and_;

		const auto auxiliaryPredicateDeclarationIsInteger =
			context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameIsInteger, 1);

		for (auto &variableDeclaration : forAll.variables)
		{
			switch (variableDeclaration->domain)
			{
				case Domain::Integer:
				{
					auto predicate = ast::Predicate(auxiliaryPredicateDeclarationIsInteger);
					predicate.arguments.reserve(1);
					predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
					and_.arguments.emplace_back(std::move(predicate));

					break;
				}
				case Domain::Program:
					variableDeclaration->domain = Domain::Union;
					break;
				default:
					throw LogicException("unexpected parameter domain, please report to bug tracker");
			}
		}
	}

	void visit(ast::Implies &implies, ast::Formula &, Context &context)
	{
		unifyDomains(implies.antecedent, context);
		unifyDomains(implies.consequent, context);
	}

	void visit(ast::In &, ast::Formula &, Context &)
	{
		throw LogicException("unexpected set inclusion operator, please report to bug tracker");
	}

	void visit(ast::Not &not_, ast::Formula &, Context &context)
	{
		unifyDomains(not_.argument, context);
	}

	void visit(ast::Or &or_, ast::Formula &, Context &context)
	{
		for (auto &argument : or_.arguments)
			unifyDomains(argument, context);
	}

	void visit(ast::Predicate &predicate, ast::Formula &, Context &context)
	{
		const auto auxiliaryFunctionDeclarationInteger
			= findOrCreateAuxiliaryUnionFunctionDeclaration(AuxiliaryFunctionNameInteger, 1, context);

		for (auto &argument : predicate.arguments)
		{
			const auto domain = unifyDomains(argument, context);

			switch (domain)
			{
				case Domain::Integer:
				{
					std::vector<ast::Term> arguments;
					arguments.reserve(1);
					arguments.emplace_back(std::move(argument));
					argument = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
				}
				case Domain::Union:
					break;
				default:
					throw LogicException("unexpected type of predicate argument, please report to bug tracker");
			}
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermUnifyDomainsVisitor
{
	Domain visit(ast::BinaryOperation &binaryOperation, ast::Term &term, Context &context)
	{
		const auto domainLeft = unifyDomains(binaryOperation.left, context);
		const auto domainRight = unifyDomains(binaryOperation.right, context);

		const auto auxiliaryFunctionNameBinaryOperation =
			[&]()
			{
				switch (binaryOperation.operator_)
				{
					case ast::BinaryOperation::Operator::Plus:
						return AuxiliaryFunctionNameSum;
					case ast::BinaryOperation::Operator::Minus:
						return AuxiliaryFunctionNameDifference;
					case ast::BinaryOperation::Operator::Multiplication:
						return AuxiliaryFunctionNameProduct;
					default:
						throw LogicException("unexpected binary operator, please report to bug tracker");
				}
			};

		const auto auxiliaryFunctionDeclarationInteger
			= findOrCreateAuxiliaryUnionFunctionDeclaration(AuxiliaryFunctionNameInteger, 1, context);

		const auto auxiliaryFunctionDeclarationBinaryOperation
			= findOrCreateAuxiliaryUnionFunctionDeclaration(auxiliaryFunctionNameBinaryOperation(), 1, context);

		// Check that both sides are of union or integer type
		if ((domainLeft != Domain::Union && domainLeft != Domain::Integer)
			|| (domainRight != Domain::Union && domainRight != Domain::Integer))
		{
			throw LogicException("unexpected type of operand of binary operation, please report to bug tracker");
		}

		// If both sides are of integer type, return
		if (domainLeft == Domain::Integer && domainRight == Domain::Integer)
			return Domain::Integer;

		// If the left side is integer, map it to a union-type object
		if (domainLeft == Domain::Integer)
		{
			std::vector<ast::Term> arguments;
			arguments.reserve(1);
			arguments.emplace_back(std::move(binaryOperation.left));
			binaryOperation.left = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
		}

		// If the right side is integer, map it to a union-type object
		if (domainRight == Domain::Integer)
		{
			std::vector<ast::Term> arguments;
			arguments.reserve(1);
			arguments.emplace_back(std::move(binaryOperation.right));
			binaryOperation.right = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
		}

		std::vector<ast::Term> arguments;
		arguments.reserve(2);
		arguments.emplace_back(std::move(binaryOperation.left));
		arguments.emplace_back(std::move(binaryOperation.right));
		term = ast::Function(auxiliaryFunctionDeclarationBinaryOperation, std::move(arguments));

		return Domain::Union;
	}

	Domain visit(ast::Boolean &, ast::Term &, Context &)
	{
		return Domain::Union;
	}

	Domain visit(ast::Function &function, ast::Term &, Context &)
	{
		if (!function.arguments.empty())
			throw TranslationException("symbolic functions not yet supported");

		function.declaration->domain = Domain::Union;

		return Domain::Union;
	}

	Domain visit(ast::Integer &integer, ast::Term &, Context &)
	{
		return Domain::Integer;
	}

	Domain visit(ast::Interval &interval, ast::Term &, Context &context)
	{
		const auto domainFrom = unifyDomains(interval.from, context);
		const auto domainTo = unifyDomains(interval.to, context);

		const auto auxiliaryFunctionDeclarationInteger
			= findOrCreateAuxiliaryUnionFunctionDeclaration(AuxiliaryFunctionNameInteger, 1, context);

		// Check that both sides are of union or integer type
		if ((domainFrom != Domain::Union && domainFrom != Domain::Integer)
			|| (domainTo != Domain::Union && domainTo != Domain::Integer))
		{
			throw LogicException("unexpected type of binary operation operand, please report to bug tracker");
		}

		// If both bounds are of integer type, return
		if (domainFrom == Domain::Integer && domainTo == Domain::Integer)
			return Domain::Integer;

		// If the left bound is integer, map it to a union-type object
		if (domainFrom == Domain::Integer)
		{
			std::vector<ast::Term> arguments;
			arguments.reserve(1);
			arguments.emplace_back(std::move(interval.from));
			interval.from = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
		}

		// If the right bound is integer but the left bound is not, map the right bound to a union-type object
		if (domainTo == Domain::Integer)
		{
			std::vector<ast::Term> arguments;
			arguments.reserve(1);
			arguments.emplace_back(std::move(interval.from));
			interval.to = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
		}

		return Domain::Union;
	}

	Domain visit(ast::SpecialInteger &, ast::Term &, Context &)
	{
		throw TranslationException("special integers not yet supported");
	}

	Domain visit(ast::String &, ast::Term &, Context &)
	{
		throw TranslationException("strings not yet supported");
	}

	Domain visit(ast::UnaryOperation &unaryOperation, ast::Term &, Context &context)
	{
		return unifyDomains(unaryOperation.argument, context);
	}

	Domain visit(ast::Variable &variable, ast::Term &, Context &)
	{
		switch (variable.declaration->domain)
		{
			case Domain::Integer:
				return Domain::Integer;
			case Domain::Program:
				return Domain::Union;
			default:
				throw LogicException("unexpected variable domain, please report to bug tracker");
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void unifyDomains(ast::Formula &formula, Context &context)
{
	return formula.accept(FormulaUnifyDomainsVisitor(), formula, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Domain unifyDomains(ast::Term &term, Context &context)
{
	return term.accept(TermUnifyDomainsVisitor(), term, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
