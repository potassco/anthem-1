#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/AST.h>
#include <anthem/ASTCopy.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/examine-semantics/Body.h>
#include <anthem/examine-semantics/Head.h>
#include <anthem/examine-semantics/Rule.h>
#include <anthem/verify-strong-equivalence/Body.h>
#include <anthem/verify-strong-equivalence/Head.h>
#include <anthem/verify-strong-equivalence/Rule.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct StatementVisitor
{
	void visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		context.logger.log(output::Priority::Debug, statement.location) << "reading program “" << program.name << "”";

		if (std::strcmp(program.name, "base") != 0)
			throw LogicException(statement.location, "program parts currently unsupported");

		if (!program.parameters.empty())
			throw LogicException(statement.location, "program parameters currently unsupported");
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
	{
		context.logger.log(output::Priority::Debug, statement.location) << "reading rule";

		switch (context.translationTarget)
		{
			case TranslationTarget::VerifyStrongEquivalence:
				verifyStrongEquivalence::translate(rule, statement, scopedFormulas, context);
				break;
			case TranslationTarget::ExamineSemantics:
				examineSemantics::translate(rule, statement, scopedFormulas, context);
				break;
		}
	}

	void visit(const Clingo::AST::ShowSignature &showSignature, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		if (showSignature.csp)
			throw LogicException(statement.location, "CSP #show statements are not supported");

		auto &signature = showSignature.signature;

		if (signature.negative())
			throw LogicException(statement.location, "negative #show atom signatures are currently unsupported");

		context.showStatementsUsed = true;
		context.defaultPredicateVisibility = ast::PredicateDeclaration::Visibility::Hidden;

		if (std::strlen(signature.name()) == 0)
		{
			context.logger.log(output::Priority::Debug, statement.location) << "showing no predicates by default";
			return;
		}

		context.logger.log(output::Priority::Debug, statement.location) << "showing “" << signature.name() << "/" << signature.arity() << "”";

		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(signature.name(), signature.arity());
		predicateDeclaration->visibility = ast::PredicateDeclaration::Visibility::Visible;
	}

	void visit(const Clingo::AST::ShowTerm &, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &)
	{
		throw LogicException(statement.location, "only #show statements for atoms (not terms) are supported currently");
	}

	void visit(const Clingo::AST::External &external, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		const auto fail =
			[&]()
			{
				throw LogicException(statement.location, "only #external declarations of the form “#external <predicate name>(<arity>).” or “#external integer(<function name>(<arity>)).” supported");
			};

		if (!external.body.empty())
			fail();

		if (!external.atom.data.is<Clingo::AST::Function>())
			fail();

		const auto &predicate = external.atom.data.get<Clingo::AST::Function>();

		if (predicate.arguments.size() != 1)
			fail();

		const auto handleIntegerDeclaration =
			[&]()
			{
				// Integer function declarations are treated separately if applicable
				if (strcmp(predicate.name, "integer") != 0)
					return false;

				if (predicate.arguments.size() != 1)
					return false;

				const auto &functionArgument = predicate.arguments.front();

				if (!functionArgument.data.is<Clingo::AST::Function>())
					return false;

				const auto &function = functionArgument.data.get<Clingo::AST::Function>();

				if (function.arguments.size() != 1)
					return false;

				const auto &arityArgument = function.arguments.front();

				if (!arityArgument.data.is<Clingo::Symbol>())
					return false;

				const auto &aritySymbol = arityArgument.data.get<Clingo::Symbol>();

				if (aritySymbol.type() != Clingo::SymbolType::Number)
					return false;

				const size_t arity = aritySymbol.number();

				auto functionDeclaration = context.findOrCreateFunctionDeclaration(function.name, arity);
				functionDeclaration->domain = Domain::Integer;

				return true;
			};

		if (handleIntegerDeclaration())
			return;

		const auto &arityArgument = predicate.arguments.front();

		if (!arityArgument.data.is<Clingo::Symbol>())
			fail();

		const auto &aritySymbol = arityArgument.data.get<Clingo::Symbol>();

		if (aritySymbol.type() != Clingo::SymbolType::Number)
			fail();

		context.externalStatementsUsed = true;

		const size_t arity = aritySymbol.number();

		auto predicateDeclaration = context.findOrCreatePredicateDeclaration(predicate.name, arity);
		predicateDeclaration->isExternal = true;
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &)
	{
		throw LogicException(statement.location, "statement currently unsupported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
