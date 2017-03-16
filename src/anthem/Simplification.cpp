#include <anthem/Simplification.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bool isPrimitiveTerm(const ast::Term &term)
{
	const auto binaryOperationIsNotPrimitiveTerm =
		[](const ast::BinaryOperationPointer &)
		{
			return false;
		};

	const auto intervalIsNotPrimitiveTerm =
		[](const ast::IntervalPointer &)
		{
			return false;
		};

	const auto defaultIsPrimitiveTerm =
		[](const auto &)
		{
			return true;
		};

	return term.match(binaryOperationIsNotPrimitiveTerm, intervalIsNotPrimitiveTerm, defaultIsPrimitiveTerm);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula simplifyFormula(ast::Formula &&formula)
{
	const auto simplifyAnd =
		[&](ast::AndPointer &and_) -> ast::Formula
		{
			for (auto &argument : and_->arguments)
				argument = simplifyFormula(std::move(argument));

			return std::move(and_);
		};

	const auto simplifyBiconditional =
		[&](ast::BiconditionalPointer &biconditional) -> ast::Formula
		{
			biconditional->left = simplifyFormula(std::move(biconditional->left));
			biconditional->right = simplifyFormula(std::move(biconditional->right));

			return std::move(biconditional);
		};

	const auto simplifyExists =
		[&](ast::ExistsPointer &exists) -> ast::Formula
		{
			exists->argument = simplifyFormula(std::move(exists->argument));

			return std::move(exists);
		};

	const auto simplifyForAll =
		[&](ast::ForAllPointer &forAll) -> ast::Formula
		{
			forAll->argument = simplifyFormula(std::move(forAll->argument));

			return std::move(forAll);
		};

	const auto simplifyImplies =
		[&](ast::ImpliesPointer &implies) -> ast::Formula
		{
			implies->antecedent = simplifyFormula(std::move(implies->antecedent));
			implies->consequent = simplifyFormula(std::move(implies->consequent));

			return std::move(implies);
		};

	const auto simplifyIn =
		[](ast::InPointer &in) -> ast::Formula
		{
			if (!isPrimitiveTerm(in->element) || !isPrimitiveTerm(in->set))
				return std::move(in);

			// Simplify formulas of type “A in B” to “A = B” if A and B are primitive
			return std::make_unique<ast::Comparison>(ast::Comparison::Operator::Equal, std::move(in->element), std::move(in->set));
		};

	const auto simplifyNot =
		[&](ast::NotPointer &not_) -> ast::Formula
		{
			not_->argument = simplifyFormula(std::move(not_->argument));

			return std::move(not_);
		};

	const auto simplifyOr =
		[&](ast::OrPointer &or_) -> ast::Formula
		{
			for (auto &argument : or_->arguments)
				argument = simplifyFormula(std::move(argument));

			return std::move(or_);
		};

	const auto defaultDoNothing =
		[&](auto &x) -> ast::Formula
		{
			return std::move(x);
		};

	return formula.match(simplifyAnd, simplifyBiconditional, simplifyExists, simplifyForAll, simplifyImplies, simplifyIn, simplifyNot, simplifyOr, defaultDoNothing);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Formula simplify(ast::Formula &&formula)
{
	return simplifyFormula(std::move(formula));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
