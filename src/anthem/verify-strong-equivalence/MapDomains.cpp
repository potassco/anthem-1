#include <anthem/verify-strong-equivalence/MapDomains.h>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Exception.h>

namespace anthem
{
namespace verifyStrongEquivalence
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MapDomains
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void mapDomains(ast::Term &term, Context &context);

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::FunctionDeclaration *findOrCreateAuxiliaryIntegerFunctionDeclaration(const char *functionName,
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
	void visit(ast::And &and_, Context &context)
	{
		for (auto &argument : and_.arguments)
			mapDomains(argument, context);
	}

	void visit(ast::Biconditional &biconditional, Context &context)
	{
		mapDomains(biconditional.left, context);
		mapDomains(biconditional.right, context);
	}

	void visit(ast::Boolean &, Context &)
	{
	}

	void visit(ast::Comparison &comparison, Context &context)
	{
		mapDomains(comparison.left, context);
		mapDomains(comparison.right, context);
	}

	void visit(ast::Exists &exists, Context &context)
	{
		mapDomains(exists.argument, context);

		if (exists.variables.empty())
			return;

		ast::And and_;

		const auto auxiliaryPredicateDeclarationIsInteger = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameIsInteger, 1);

		for (auto &variableDeclaration : exists.variables)
		{
			if (variableDeclaration->domain == Domain::Unknown)
				throw TranslationException("unexpected unknown parameter domain, please report to bug tracker");

			if (variableDeclaration->domain == Domain::Integer)
			{
				auto predicate = ast::Predicate(auxiliaryPredicateDeclarationIsInteger);
				predicate.arguments.reserve(1);
				predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
				and_.arguments.emplace_back(std::move(predicate));
			}

			variableDeclaration->domain = Domain::Integer;
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
		mapDomains(forAll.argument, context);

		if (forAll.variables.empty())
			return;

		ast::And and_;

		const auto auxiliaryPredicateDeclarationIsInteger = context.findOrCreatePredicateDeclaration(AuxiliaryPredicateNameIsInteger, 1);

		for (auto &variableDeclaration : forAll.variables)
		{
			if (variableDeclaration->domain == Domain::Unknown)
				throw TranslationException("unexpected unknown parameter domain, please report to bug tracker");

			if (variableDeclaration->domain == Domain::Integer)
			{
				auto predicate = ast::Predicate(auxiliaryPredicateDeclarationIsInteger);
				predicate.arguments.reserve(1);
				predicate.arguments.emplace_back(ast::Variable(variableDeclaration.get()));
				and_.arguments.emplace_back(std::move(predicate));
			}

			variableDeclaration->domain = Domain::Integer;
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
		mapDomains(implies.antecedent, context);
		mapDomains(implies.consequent, context);
	}

	void visit(ast::In &in, Context &context)
	{
		mapDomains(in.element, context);
		mapDomains(in.set, context);
	}

	void visit(ast::Not &not_, Context &context)
	{
		mapDomains(not_.argument, context);
	}

	void visit(ast::Or &or_, Context &context)
	{
		for (auto &argument : or_.arguments)
			mapDomains(argument, context);
	}

	void visit(ast::Predicate &predicate, Context &context)
	{
		for (auto &argument : predicate.arguments)
			mapDomains(argument, context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermMapDomainsVisitor
{
	void visit(ast::BinaryOperation &binaryOperation, ast::Term &, Context &context)
	{
		mapDomains(binaryOperation.left, context);
		mapDomains(binaryOperation.right, context);

		// For the multiplication, division, and remainder operations, special care is necessary because
		// all integers are already multiplied by 2
		switch (binaryOperation.operator_)
		{
			case ast::BinaryOperation::Operator::Plus:
			case ast::BinaryOperation::Operator::Minus:
			case ast::BinaryOperation::Operator::Multiplication:
				break;
			// TODO: implement
			case ast::BinaryOperation::Operator::Division:
				throw TranslationException("division not yet supported in domain mapping");
			case ast::BinaryOperation::Operator::Modulo:
				throw TranslationException("modulo not yet supported in domain mapping");
			case ast::BinaryOperation::Operator::Power:
				throw TranslationException("power operator not yet supported in domain mapping");
		}
	}

	void visit(ast::Boolean &, ast::Term &, Context &)
	{
	}

	void visit(ast::Function &function, ast::Term &term, Context &context)
	{
		if (!function.arguments.empty())
			throw TranslationException("mapping symbolic functions to odd integers not yet implemented");

		function.declaration->domain = Domain::Symbolic;

		std::vector<ast::Term> arguments;
		arguments.reserve(1);
		arguments.emplace_back(std::move(function));

		auto auxiliaryFunctionDeclarationSymbolic
			= findOrCreateAuxiliaryIntegerFunctionDeclaration(AuxiliaryFunctionNameSymbolic, 1, context);

		term = ast::Function(auxiliaryFunctionDeclarationSymbolic, std::move(arguments));
	}

	void visit(ast::Integer &integer, ast::Term &term, Context &context)
	{
		std::vector<ast::Term> arguments;
		arguments.reserve(1);
		arguments.emplace_back(std::move(integer));

		auto auxiliaryFunctionDeclarationInteger
			= findOrCreateAuxiliaryIntegerFunctionDeclaration(AuxiliaryFunctionNameInteger, 1, context);

		term = ast::Function(auxiliaryFunctionDeclarationInteger, std::move(arguments));
	}

	void visit(ast::Interval &interval, ast::Term &, Context &context)
	{
		mapDomains(interval.from, context);
		mapDomains(interval.to, context);
	}

	void visit(ast::SpecialInteger &, ast::Term &, Context &)
	{
		throw TranslationException("special integers not yet supported in domain mapping");
	}

	void visit(ast::String &, ast::Term &, Context &)
	{
		throw TranslationException("strings not yet supported in domain mapping");
	}

	void visit(ast::UnaryOperation &unaryOperation, ast::Term &, Context &context)
	{
		// TODO: check
		mapDomains(unaryOperation.argument, context);
	}

	void visit(ast::Variable &, ast::Term &, Context &)
	{
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
	term.accept(TermMapDomainsVisitor(), term, context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
