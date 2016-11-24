#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/Body.h>
#include <anthem/Head.h>
#include <anthem/Utils.h>
#include <anthem/output/ClingoOutput.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

struct StatementVisitor
{
	void visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement, Context &context)
	{
		// TODO: refactor
		context.logger.log(output::Priority::Debug, (std::string("[program] ") + program.name).c_str());

		if (!program.parameters.empty())
			throwErrorAtLocation(statement.location, "program parameters currently unsupported", context);
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &, Context &context)
	{
		// TODO: implement more nicely
		context.headTerms.clear();

		auto &outputStream = context.logger.outputStream();

		// Concatenate all head terms
		rule.head.data.accept(HeadLiteralCollectFunctionTermsVisitor(), rule.head, context);

		// Print auxiliary variables replacing the head atom’s arguments
		if (!context.headTerms.empty())
		{
			for (auto i = context.headTerms.cbegin(); i != context.headTerms.cend(); i++)
			{
				const auto &headTerm = **i;

				if (i != context.headTerms.cbegin())
					outputStream << " " << Clingo::AST::BinaryOperator::And << " ";

				const auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(i - context.headTerms.cbegin() + 1);

				outputStream
					<< output::Variable(variableName.c_str())
					<< " " << output::Keyword("in") << " " << headTerm;
			}
		}

		if (rule.body.empty() && context.headTerms.empty())
			outputStream << Clingo::AST::Boolean({true});
		else
		{
			// Print translated body literals
			for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
			{
				const auto &bodyLiteral = *i;

				if (!context.headTerms.empty())
					outputStream << " " << Clingo::AST::BinaryOperator::And << " ";

				if (bodyLiteral.sign != Clingo::AST::Sign::None)
					throwErrorAtLocation(bodyLiteral.location, "only positive literals currently supported", context);

				bodyLiteral.data.accept(BodyLiteralPrintVisitor(), bodyLiteral, context);
			}
		}

		outputStream << " " << output::Operator("->") << " ";

		// Print consequent of the implication
		rule.head.data.accept(HeadLiteralPrintSubstitutedVisitor(), rule.head, context);

		outputStream << std::endl;
	}

	void visit(const Clingo::AST::Definition &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“definition” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::ShowSignature &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“show signature” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::ShowTerm &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“show term” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::Minimize &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“minimize” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::Script &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“script” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::External &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“external” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::Edge &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“edge” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::Heuristic &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“heuristic” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::ProjectAtom &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“project atom” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::ProjectSignature &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“project signature” statements currently unsupported", context);
	}

	void visit(const Clingo::AST::TheoryDefinition &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "“theory definition” statements currently unsupported", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
