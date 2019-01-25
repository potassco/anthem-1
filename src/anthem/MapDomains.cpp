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

void mapDomains(Term &term, Context &context);

////////////////////////////////////////////////////////////////////////////////////////////////////

FunctionDeclaration *findOrCreateAuxiliaryIntegerFunctionDeclaration(const char *functionName,
	size_t arity, Context &context)
{
	auto auxiliaryIntegerFunctionDeclaration
		= context.findOrCreateFunctionDeclaration(functionName, arity);

	auxiliaryIntegerFunctionDeclaration->domain = Domain::Integer;

	return auxiliaryIntegerFunctionDeclaration;
}

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

		And and_;

		const auto auxiliaryPredicateDeclarationEven = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameEven, 1);

		for (auto &variableDeclaration : exists.variables)
		{
			if (variableDeclaration->domain == Domain::Unknown)
				throw TranslationException("unexpected unknown parameter domain, please report to bug tracker");

			if (variableDeclaration->domain == Domain::Integer)
			{
				auto predicate = Predicate(auxiliaryPredicateDeclarationEven);
				predicate.arguments.reserve(1);
				predicate.arguments.emplace_back(Variable(variableDeclaration.get()));
				and_.arguments.emplace_back(std::move(predicate));
			}

			variableDeclaration->domain = Domain::Integer;
		}

		if (and_.arguments.empty())
			return;

		if (and_.arguments.size() == 1)
		{
			Implies implies(std::move(and_.arguments.front()), std::move(exists.argument));
			exists.argument = std::move(implies);
			return;
		}

		Implies implies(std::move(and_), std::move(exists.argument));
		exists.argument = std::move(implies);
	}

	// TODO: avoid code duplication
	void visit(ForAll &forAll, Context &context)
	{
		mapDomains(forAll.argument, context);

		if (forAll.variables.empty())
			return;

		And and_;

		const auto auxiliaryPredicateDeclarationEven = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameEven, 1);

		for (auto &variableDeclaration : forAll.variables)
		{
			if (variableDeclaration->domain == Domain::Unknown)
				throw TranslationException("unexpected unknown parameter domain, please report to bug tracker");

			if (variableDeclaration->domain == Domain::Integer)
			{
				auto predicate = Predicate(auxiliaryPredicateDeclarationEven);
				predicate.arguments.reserve(1);
				predicate.arguments.emplace_back(Variable(variableDeclaration.get()));
				and_.arguments.emplace_back(std::move(predicate));
			}

			variableDeclaration->domain = Domain::Integer;
		}

		if (and_.arguments.empty())
			return;

		if (and_.arguments.size() == 1)
		{
			Implies implies(std::move(and_.arguments.front()), std::move(forAll.argument));
			forAll.argument = std::move(implies);
			return;
		}

		Implies implies(std::move(and_), std::move(forAll.argument));
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
	void visit(BinaryOperation &binaryOperation, Term &term, Context &context)
	{
		mapDomains(binaryOperation.left, context);
		mapDomains(binaryOperation.right, context);

		// For the multiplication, division, and remainder operations, special care is necessary because
		// all integers are already multiplied by 2
		switch (binaryOperation.operator_)
		{
			// Addition and subtraction are not affected by the domain mapping
			case BinaryOperation::Operator::Plus:
			case BinaryOperation::Operator::Minus:
				break;
			case BinaryOperation::Operator::Multiplication:
			{
				auto auxiliaryMapIntegerFunctionDeclaration
					= findOrCreateAuxiliaryIntegerFunctionDeclaration(AuxiliaryFunctionNameMapInteger, 1, context);
				auto auxiliaryUnmapIntegerFunctionDeclaration
					= findOrCreateAuxiliaryIntegerFunctionDeclaration(AuxiliaryFunctionNameUnmapInteger, 1, context);

				std::vector<Term> leftArguments;
				leftArguments.reserve(1);
				leftArguments.emplace_back(std::move(binaryOperation.left));

				std::vector<Term> rightArguments;
				rightArguments.reserve(1);
				rightArguments.emplace_back(std::move(binaryOperation.right));

				auto left = Function(auxiliaryUnmapIntegerFunctionDeclaration, std::move(leftArguments));
				auto right = Function(auxiliaryUnmapIntegerFunctionDeclaration, std::move(rightArguments));

				auto multiply = BinaryOperation(BinaryOperation::Operator::Multiplication, std::move(left), std::move(right));

				std::vector<Term> arguments;
				arguments.reserve(1);
				arguments.emplace_back(std::move(multiply));

				term = Function(auxiliaryMapIntegerFunctionDeclaration, std::move(arguments));
				return;
			}
			// TODO: implement
			case BinaryOperation::Operator::Division:
				throw TranslationException("division not yet supported in domain mapping");
			case BinaryOperation::Operator::Modulo:
				throw TranslationException("modulo not yet supported in domain mapping");
			case BinaryOperation::Operator::Power:
				throw TranslationException("power operator not yet supported in domain mapping");
		}
	}

	void visit(Boolean &, Term &, Context &)
	{
	}

	void visit(Function &function, Term &, Context &context)
	{
		// TODO: implement
		throw TranslationException("mapping symbolic functions to odd integers not yet implemented");

		for (auto &argument : function.arguments)
			mapDomains(argument, context);
	}

	void visit(Integer &integer, Term &term, Context &context)
	{
		std::vector<Term> arguments;
		arguments.reserve(1);
		arguments.emplace_back(std::move(integer));

		auto auxiliaryMapIntegerFunctionDeclaration
			= findOrCreateAuxiliaryIntegerFunctionDeclaration(AuxiliaryFunctionNameMapInteger, 1, context);

		term = Function(auxiliaryMapIntegerFunctionDeclaration, std::move(arguments));
	}

	void visit(Interval &interval, Term &, Context &context)
	{
		mapDomains(interval.from, context);
		mapDomains(interval.to, context);
	}

	void visit(SpecialInteger &, Term &, Context &)
	{
		throw TranslationException("special integers not yet supported in domain mapping");
	}

	void visit(String &, Term &, Context &)
	{
		throw TranslationException("strings not yet supported in domain mapping");
	}

	void visit(UnaryOperation &unaryOperation, Term &, Context &context)
	{
		// TODO: check
		mapDomains(unaryOperation.argument, context);
	}

	void visit(Variable &, Term &, Context &)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void mapDomains(Formula &formula, Context &context)
{
	formula.accept(FormulaMapDomainsVisitor(), context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void mapDomains(Term &term, Context &context)
{
	term.accept(TermMapDomainsVisitor(), term, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
