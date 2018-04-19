#ifndef __ANTHEM__AST_COPY_H
#define __ANTHEM__AST_COPY_H

#include <anthem/AST.h>
#include <anthem/ASTVisitors.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ASTCopy
//
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Preparing Deep Copying
////////////////////////////////////////////////////////////////////////////////////////////////////

BinaryOperation prepareCopy(const BinaryOperation &other);
Boolean prepareCopy(const Boolean &other);
Comparison prepareCopy(const Comparison &other);
Function prepareCopy(const Function &other);
In prepareCopy(const In &other);
Integer prepareCopy(const Integer &other);
Interval prepareCopy(const Interval &other);
Predicate prepareCopy(const Predicate &other);
SpecialInteger prepareCopy(const SpecialInteger &other);
String prepareCopy(const String &other);
Variable prepareCopy(const Variable &other);
VariableDeclaration prepareCopy(const VariableDeclaration &other);
VariableDeclarationPointers prepareCopy(const VariableDeclarationPointers &other);
And prepareCopy(const And &other);
Biconditional prepareCopy(const Biconditional &other);
Exists prepareCopy(const Exists &other);
ForAll prepareCopy(const ForAll &other);
Implies prepareCopy(const Implies &other);
Not prepareCopy(const Not &other);
Or prepareCopy(const Or &other);

Term prepareCopy(const Term &term);
std::vector<Term> prepareCopy(const std::vector<Term> &terms);
Formula prepareCopy(const Formula &formula);
std::vector<Formula> prepareCopy(const std::vector<Formula> &formulas);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Fixing Dangling Variables
////////////////////////////////////////////////////////////////////////////////////////////////////

void fixDanglingVariables(ScopedFormula &scopedFormula);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
