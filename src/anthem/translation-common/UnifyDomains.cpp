#include <anthem/translation-common/UnifyDomains.h>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Exception.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// UnifyDomains
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void unifyDomains(ast::Term &term, Context &context);

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
	void visit(ast::And &and_, Context &context)
	{
		for (auto &argument : and_.arguments)
			unifyDomains(argument, context);
	}

	void visit(ast::Biconditional &biconditional, Context &context)
	{
		unifyDomains(biconditional.left, context);
		unifyDomains(biconditional.right, context);
	}

	void visit(ast::Boolean &, Context &)
	{
	}

	void visit(ast::Comparison &comparison, Context &context)
	{
		unifyDomains(comparison.left, context);
		unifyDomains(comparison.right, context);
	}

	void visit(ast::Exists &exists, Context &context)
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
					break;
				default:
					throw LogicException("unexpected parameter domain, please report to bug tracker");
			}

			variableDeclaration->domain = Domain::Union;
		}

		and_.arguments.emplace_back(std::move(exists.argument));

		if (and_.arguments.empty())
			return;

		if (and_.arguments.size() == 1)
		{
			exists.argument = std::move(and_.arguments.front());
			return;
		}

		exists.argument = std::move(and_);
	}

	void visit(ast::ForAll &forAll, Context &context)
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
					break;
				default:
					throw LogicException("unexpected parameter domain, please report to bug tracker");
			}

			variableDeclaration->domain = Domain::Union;
		}

		if (and_.arguments.empty())
			return;

		if (and_.arguments.size() == 1)
		{
			ast::Implies implies(std::move(and_.arguments.front()), std::move(forAll.argument));
			forAll.argument = std::move(implies);
			return;
		}

		ast::Implies implies(std::move(and_), std::move(forAll.argument));
		forAll.argument = std::move(implies);
	}

	void visit(ast::Implies &implies, Context &context)
	{
		unifyDomains(implies.antecedent, context);
		unifyDomains(implies.consequent, context);
	}

	void visit(ast::In &in, Context &context)
	{
		unifyDomains(in.element, context);
		unifyDomains(in.set, context);
	}

	void visit(ast::Not &not_, Context &context)
	{
		unifyDomains(not_.argument, context);
	}

	void visit(ast::Or &or_, Context &context)
	{
		for (auto &argument : or_.arguments)
			unifyDomains(argument, context);
	}

	void visit(ast::Predicate &predicate, Context &context)
	{
		for (auto &argument : predicate.arguments)
			unifyDomains(argument, context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermUnifyDomainsVisitor
{
	void visit(ast::BinaryOperation &binaryOperation, ast::Term &, Context &context)
	{
		unifyDomains(binaryOperation.left, context);
		unifyDomains(binaryOperation.right, context);

		switch (binaryOperation.operator_)
		{
			case ast::BinaryOperation::Operator::Plus:
			case ast::BinaryOperation::Operator::Minus:
			case ast::BinaryOperation::Operator::Multiplication:
				break;
			default:
				throw LogicException("unexpected binary operation, please report to bug tracker");
		}
	}

	void visit(ast::Boolean &, ast::Term &, Context &)
	{
	}

	void visit(ast::Function &function, ast::Term &term, Context &context)
	{
		if (!function.arguments.empty())
			throw TranslationException("symbolic functions not yet supported");

		function.declaration->domain = Domain::Union;
	}

	void visit(ast::Integer &integer, ast::Term &term, Context &context)
	{
		std::vector<ast::Term> arguments;
		arguments.reserve(1);
		arguments.emplace_back(std::move(integer));

		auto auxiliaryFunctionDeclarationInteger
			= findOrCreateAuxiliaryUnionFunctionDeclaration(AuxiliaryFunctionNameInteger, 1, context);

		term = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
	}

	void visit(ast::Interval &interval, ast::Term &, Context &context)
	{
		unifyDomains(interval.from, context);
		unifyDomains(interval.to, context);
	}

	void visit(ast::SpecialInteger &, ast::Term &, Context &)
	{
		throw TranslationException("special integers not yet supported");
	}

	void visit(ast::String &, ast::Term &, Context &)
	{
		throw TranslationException("strings not yet supported");
	}

	void visit(ast::UnaryOperation &unaryOperation, ast::Term &, Context &context)
	{
		unifyDomains(unaryOperation.argument, context);
	}

	void visit(ast::Variable &, ast::Term &, Context &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void unifyDomains(ast::Formula &formula, Context &context)
{
	formula.accept(FormulaUnifyDomainsVisitor(), context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void unifyDomains(ast::Term &term, Context &context)
{
	term.accept(TermUnifyDomainsVisitor(), term, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
