#ifndef __ANTHEM__OUTPUT__CLINGO_OUTPUT_H
#define __ANTHEM__OUTPUT__CLINGO_OUTPUT_H

#include <clingo.hh>

#include <anthem/output/ColorStream.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ClingoOutput
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ColorStream &operator<<(ColorStream &stream, const Clingo::Symbol &symbol);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Sign &sign);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Boolean &boolean);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Variable &variable);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::BinaryOperator &binaryOperator);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::UnaryOperation &unaryOperation);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::BinaryOperation &binaryOperation);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Interval &interval);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Function &function);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Pool &pool);
ColorStream &operator<<(ColorStream &stream, const Clingo::AST::Term &term);

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
