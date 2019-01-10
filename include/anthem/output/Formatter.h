#ifndef __ANTHEM__OUTPUT__FORMATTER_H
#define __ANTHEM__OUTPUT__FORMATTER_H

#include <map>

#include <anthem/AST.h>
#include <anthem/output/ColorStream.h>

namespace anthem
{
namespace output
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct PrintContext
{
	PrintContext(const Context &context)
	:	context{context}
	{
	}

	PrintContext(const PrintContext &other) = delete;
	PrintContext &operator=(const PrintContext &other) = delete;
	PrintContext(PrintContext &&other) = delete;
	PrintContext &operator=(PrintContext &&other) = delete;

	std::map<const ast::VariableDeclaration *, size_t> userVariableIDs;
	std::map<const ast::VariableDeclaration *, size_t> headVariableIDs;
	std::map<const ast::VariableDeclaration *, size_t> bodyVariableIDs;
	std::map<const ast::VariableDeclaration *, size_t> integerVariableIDs;
	size_t currentFormulaID{0};

	const Context &context;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Formatter, typename Type>
output::ColorStream &print(output::ColorStream &stream, const Type &value, PrintContext &printContext, bool omitParentheses = false)
{
	return Formatter::print(stream, value, printContext, omitParentheses);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Formatter, class Variant>
struct VariantPrintVisitor
{
	template<class T>
	output::ColorStream &visit(const T &x, output::ColorStream &stream, PrintContext &printContext, bool omitParentheses)
	{
		return Formatter::print(stream, x, printContext, omitParentheses);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
