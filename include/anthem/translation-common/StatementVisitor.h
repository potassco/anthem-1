#ifndef __ANTHEM__TRANSLATION_COMMON__STATEMENT_VISITOR_H
#define __ANTHEM__TRANSLATION_COMMON__STATEMENT_VISITOR_H

#include <clingo.hh>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename ReadFunctor, class TranslationContext>
struct StatementVisitor
{
	void visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement,
		ReadFunctor &, Context &context, TranslationContext &)
	{
		context.logger.log(output::Priority::Debug, statement.location) << "reading program “" << program.name << "”";

		if (std::strcmp(program.name, "base") != 0)
			throw TranslationException(statement.location, "program parts not yet supported");

		if (!program.parameters.empty())
			throw TranslationException(statement.location, "program parameters not yet supported");
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &statement,
		ReadFunctor &readFunctor, Context &context, TranslationContext &)
	{
		context.logger.log(output::Priority::Debug, statement.location) << "reading rule";

		readFunctor(rule);
	}

	void visit(const Clingo::AST::ShowSignature &showSignature,
		const Clingo::AST::Statement &statement, ReadFunctor &, Context &context, TranslationContext &)
	{
		if (showSignature.csp)
			throw TranslationException(statement.location, "CSP #show statements not yet supported");

		auto &signature = showSignature.signature;

		if (signature.negative())
			throw TranslationException(statement.location, "negative #show atom signatures not yet supported");

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

	void visit(const Clingo::AST::ShowTerm &, const Clingo::AST::Statement &statement,
		ReadFunctor &, Context &, TranslationContext &)
	{
		throw TranslationException(statement.location, "only #show statements for atoms (not terms) currently supported");
	}

	void visit(const Clingo::AST::External &external, const Clingo::AST::Statement &statement,
		ReadFunctor &, Context &context, TranslationContext &)
	{
		const auto fail =
			[&]()
			{
				throw TranslationException(statement.location, "only #external declarations of the form “#external <predicate name>(<arity>).” or “#external integer(<function name>(<arity>)).” currently supported");
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
	void visit(const T &, const Clingo::AST::Statement &statement, ReadFunctor &, Context &,
		TranslationContext &)
	{
		throw TranslationException(statement.location, "statement not yet supported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
