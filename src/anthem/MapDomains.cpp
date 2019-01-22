#include <anthem/MapDomains.h>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Exception.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MapDomains
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void mapDomains(ast::Term &term, Context &context);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct FormulaMapDomainsVisitor
{
	void visit(And &and_, Context &context)
	{
		for (auto &argument : and_.arguments)
			mapDomains(argument, context);
	}

	void visit(Biconditional &biconditional, Context &context)
	{
		mapDomains(biconditional.left, context);
		mapDomains(biconditional.right, context);
	}

	void visit(Boolean &, Context &)
	{
	}

	void visit(Comparison &comparison, Context &context)
	{
		mapDomains(comparison.left, context);
		mapDomains(comparison.right, context);
	}

	void visit(Exists &exists, Context &context)
	{
		mapDomains(exists.argument, context);

		if (exists.variables.empty())
			return;

		// TODO: omit unnecessary conjunction if only one argument is present
		ast::And and_;
		and_.arguments.reserve(exists.variables.size());

		const auto auxiliaryPredicateDeclarationEven = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameEven, 1);
		const auto auxiliaryPredicateDeclarationOdd = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameOdd, 1);

		for (const auto &variableDeclaration : exists.variables)
			switch (variableDeclaration->domain)
			{
				case Domain::Symbolic:
				{
					auto predicate = ast::Predicate(auxiliaryPredicateDeclarationOdd);
					predicate.arguments.reserve(1);
					predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
					and_.arguments.emplace_back(std::move(predicate));
					break;
				}
				case Domain::Integer:
				{
					auto predicate = ast::Predicate(auxiliaryPredicateDeclarationEven);
					predicate.arguments.reserve(1);
					predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
					and_.arguments.emplace_back(std::move(predicate));
					break;
				}
				default:
					throw TranslationException("unexpected unknown parameter domain, please report to bug tracker");
			}

		ast::Implies implies(std::move(and_), std::move(exists.argument));
		exists.argument = std::move(implies);
	}

	void visit(ForAll &forAll, Context &context)
	{
		mapDomains(forAll.argument, context);

		if (forAll.variables.empty())
			return;

		// TODO: omit unnecessary conjunction if only one argument is present
		ast::And and_;
		and_.arguments.reserve(forAll.variables.size());

		auto auxiliaryPredicateDeclarationEven = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameEven, 1);
		auto auxiliaryPredicateDeclarationOdd = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameOdd, 1);

		for (const auto &variableDeclaration : forAll.variables)
			switch (variableDeclaration->domain)
			{
				case Domain::Symbolic:
				{
					auto predicate = ast::Predicate(auxiliaryPredicateDeclarationOdd);
					predicate.arguments.reserve(1);
					predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
					and_.arguments.emplace_back(std::move(predicate));
					break;
				}
				case Domain::Integer:
				{
					auto predicate = ast::Predicate(auxiliaryPredicateDeclarationEven);
					predicate.arguments.reserve(1);
					predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
					and_.arguments.emplace_back(std::move(predicate));
					break;
				}
				default:
					throw TranslationException("unexpected unknown parameter domain, please report to bug tracker");
			}

		ast::Implies implies(std::move(and_), std::move(forAll.argument));
		forAll.argument = std::move(implies);
	}

	void visit(Implies &implies, Context &context)
	{
		mapDomains(implies.antecedent, context);
		mapDomains(implies.consequent, context);
	}

	void visit(In &in, Context &context)
	{
		mapDomains(in.element, context);
		mapDomains(in.set, context);
	}

	void visit(Not &not_, Context &context)
	{
		mapDomains(not_.argument, context);
	}

	void visit(Or &or_, Context &context)
	{
		for (auto &argument : or_.arguments)
			mapDomains(argument, context);
	}

	void visit(Predicate &predicate, Context &context)
	{
		for (auto &argument : predicate.arguments)
			mapDomains(argument, context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermMapDomainsVisitor
{
	void visit(BinaryOperation &binaryOperation, Context &context)
	{
		mapDomains(binaryOperation.left, context);
		mapDomains(binaryOperation.right, context);

		// For the multiplication, division, and remainder operations, special care is necessary because
		// all integers are already multiplied by 2
		switch (binaryOperation.operator_)
		{
			case BinaryOperation::Operator::Plus:
			case BinaryOperation::Operator::Minus:
				break;
			// TODO: implement
			case BinaryOperation::Operator::Multiplication:
				throw TranslationException("multiplication not yet supported in domain mapping");
			case BinaryOperation::Operator::Division:
				throw TranslationException("division not yet supported in domain mapping");
			case BinaryOperation::Operator::Modulo:
				throw TranslationException("modulo not yet supported in domain mapping");
			case BinaryOperation::Operator::Power:
				throw TranslationException("power operator not yet supported in domain mapping");
		}
	}

	void visit(Boolean &, Context &)
	{
	}

	void visit(Function &function, Context &context)
	{
		// TODO: implement
		throw TranslationException("mapping symbolic functions to odd integers not yet implemented");

		for (auto &argument : function.arguments)
			mapDomains(argument, context);
	}

	void visit(Integer &integer, Context &)
	{
		// Integers n are mapped to 2 * n
		integer.value *= 2;
	}

	void visit(Interval &interval, Context &context)
	{
		mapDomains(interval.from, context);
		mapDomains(interval.to, context);
	}

	void visit(SpecialInteger &, Context &)
	{
		throw TranslationException("special integers not yet supported in domain mapping");
	}

	void visit(String &, Context &)
	{
		throw TranslationException("strings not yet supported in domain mapping");
	}

	void visit(UnaryOperation &unaryOperation, Context &context)
	{
		// TODO: check
		mapDomains(unaryOperation.argument, context);
	}

	void visit(Variable &variable, Context &)
	{
		// As a result of the domain mapping, all variables are integer
		variable.declaration->domain = Domain::Integer;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void mapDomains(ast::Formula &formula, Context &context)
{
	formula.accept(FormulaMapDomainsVisitor(), context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void mapDomains(ast::Term &term, Context &context)
{
	term.accept(TermMapDomainsVisitor(), context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
