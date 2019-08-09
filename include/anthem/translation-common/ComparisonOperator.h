#ifndef __ANTHEM__TRANSLATION_COMMON__COMPARISON_OPERATOR_H
#define __ANTHEM__TRANSLATION_COMMON__COMPARISON_OPERATOR_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/Exception.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ComparisonOperator
//
////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::Comparison::Operator translate(Clingo::AST::ComparisonOperator comparisonOperator)
{
	switch (comparisonOperator)
	{
		case Clingo::AST::ComparisonOperator::GreaterThan:
			return ast::Comparison::Operator::GreaterThan;
		case Clingo::AST::ComparisonOperator::LessThan:
			return ast::Comparison::Operator::LessThan;
		case Clingo::AST::ComparisonOperator::LessEqual:
			return ast::Comparison::Operator::LessEqual;
		case Clingo::AST::ComparisonOperator::GreaterEqual:
			return ast::Comparison::Operator::GreaterEqual;
		case Clingo::AST::ComparisonOperator::NotEqual:
			return ast::Comparison::Operator::NotEqual;
		case Clingo::AST::ComparisonOperator::Equal:
			return ast::Comparison::Operator::Equal;
	}

	throw TranslationException("unknown comparison operator, please report to bug tracker");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
