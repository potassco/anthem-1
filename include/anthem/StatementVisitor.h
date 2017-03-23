#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/AST.h>
#include <anthem/Body.h>
#include <anthem/Head.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct StatementVisitor
{
	std::vector<ast::Formula> visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement, Context &context)
	{
		// TODO: refactor
		context.logger.log(output::Priority::Debug, (std::string("[program] ") + program.name).c_str());

		if (!program.parameters.empty())
			throwErrorAtLocation(statement.location, "program parameters currently unsupported", context);

		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &, Context &context)
	{
		context.reset();

		// Concatenate all head terms
		rule.head.data.accept(HeadLiteralCollectFunctionTermsVisitor(), rule.head, context);

		ast::And antecedent;

		// Compute consequent
		auto consequent = rule.head.data.accept(HeadLiteralTranslateToConsequentVisitor(), rule.head, context);

		if (!consequent)
		{
			context.logger.log(output::Priority::Error, "could not translate formula consequent");
			return {};
		}

		// Print auxiliary variables replacing the head atom’s arguments
		for (auto i = context.headTerms.cbegin(); i != context.headTerms.cend(); i++)
		{
			const auto &headTerm = **i;

			auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(i - context.headTerms.cbegin() + 1);
			auto element = ast::Variable(std::move(variableName), ast::Variable::Type::Reserved);
			auto set = translate(headTerm, context);
			auto in = ast::In(std::move(element), std::move(set));

			antecedent.arguments.emplace_back(std::move(in));
		}

		// Print translated body literals
		for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
		{
			const auto &bodyLiteral = *i;

			if (bodyLiteral.sign != Clingo::AST::Sign::None)
				throwErrorAtLocation(bodyLiteral.location, "only positive literals currently supported", context);

			auto argument = bodyLiteral.data.accept(BodyBodyLiteralTranslateVisitor(), bodyLiteral, context);

			if (!argument)
				throwErrorAtLocation(bodyLiteral.location, "could not translate body literal", context);

			antecedent.arguments.emplace_back(std::move(argument.value()));
		}

		// Handle choice rules
		if (context.isChoiceRule)
			antecedent.arguments.emplace_back(ast::deepCopy(consequent.value()));

		// Use “true” as the consequent in case it is empty
		if (antecedent.arguments.empty())
			return {ast::Formula::make<ast::Implies>(ast::Boolean(true), std::move(consequent.value()))};
		else if (antecedent.arguments.size() == 1)
			return {ast::Formula::make<ast::Implies>(std::move(antecedent.arguments[0]), std::move(consequent.value()))};

		return {ast::Formula::make<ast::Implies>(std::move(antecedent), std::move(consequent.value()))};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::Definition &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“definition” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::ShowSignature &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“show signature” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::ShowTerm &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“show term” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::Minimize &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“minimize” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::Script &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“script” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::External &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“external” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::Edge &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“edge” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::Heuristic &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“heuristic” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::ProjectAtom &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“project atom” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::ProjectSignature &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“project signature” statements currently unsupported", context);
		return {};
	}

	std::vector<ast::Formula> visit(const Clingo::AST::TheoryDefinition &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“theory definition” statements currently unsupported", context);
		return {};
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
