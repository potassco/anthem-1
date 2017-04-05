#include <anthem/Completion.h>

#include <anthem/ASTVisitors.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Completion
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void complete(std::vector<ast::Formula> &formulas)
{
	for (auto &formula : formulas)
	{
		if (!formula.is<ast::Implies>())
			throw std::runtime_error("cannot perform completion, formula not in normal form");

		auto &implies = formula.get<ast::Implies>();

		if (!implies.consequent.is<ast::Predicate>())
			throw std::runtime_error("cannot perform completion, only single predicates supported as formula consequent currently");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
