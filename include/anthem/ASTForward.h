#ifndef __ANTHEM__AST_FORWARD_H
#define __ANTHEM__AST_FORWARD_H

#include <memory>
#include <experimental/optional>
#include <mapbox/variant.hpp>
#include <vector>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST Forward Declarations
//
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////

struct And;
struct BinaryOperation;
struct Biconditional;
struct Boolean;
struct Constant;
struct Equals;
struct Exists;
struct ForAll;
struct Function;
struct Implies;
struct Integer;
struct IntegerRange;
struct Not;
struct Or;
struct Predicate;
struct Variable;

using AndPointer = std::unique_ptr<And>;
using BinaryOperationPointer = std::unique_ptr<BinaryOperation>;
using BiconditionalPointer = std::unique_ptr<Biconditional>;
using BooleanPointer = std::unique_ptr<Boolean>;
using ConstantPointer = std::unique_ptr<Constant>;
using EqualsPointer = std::unique_ptr<Equals>;
using ExistsPointer = std::unique_ptr<Exists>;
using ForAllPointer = std::unique_ptr<ForAll>;
using FunctionPointer = std::unique_ptr<Function>;
using ImpliesPointer = std::unique_ptr<Implies>;
using IntegerPointer = std::unique_ptr<Integer>;
using IntegerRangePointer = std::unique_ptr<IntegerRange>;
using NotPointer = std::unique_ptr<Not>;
using OrPointer = std::unique_ptr<Or>;
using PredicatePointer = std::unique_ptr<Predicate>;
using VariablePointer = std::unique_ptr<Variable>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Variants
////////////////////////////////////////////////////////////////////////////////////////////////////

using TermT = mapbox::util::variant<
	BinaryOperationPointer,
	ConstantPointer,
	IntegerPointer,
	FunctionPointer,
	VariablePointer>;

class Term : public TermT
{
	using TermT::TermT;
};

using FormulaT = mapbox::util::variant<
	AndPointer,
	BiconditionalPointer,
	BooleanPointer,
	EqualsPointer,
	ExistsPointer,
	ForAllPointer,
	ImpliesPointer,
	NotPointer,
	OrPointer,
	PredicatePointer>;

class Formula : public FormulaT
{
	using FormulaT::FormulaT;
};

using Formulas = std::vector<Formula>;

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
