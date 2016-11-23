#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/Body.h>
#include <anthem/Head.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void throwErrorUnsupportedStatement(const char *statementType, const Clingo::AST::Statement &statement)
{
	const auto errorMessage = std::string("“") + statementType + "” statements currently unsupported";

	throwErrorAtLocation(statement.location, errorMessage.c_str());

	throw std::runtime_error(errorMessage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct StatementVisitor
{
	void visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement)
	{
		std::cout << "[program] " << program.name << std::endl;

		if (!program.parameters.empty())
			throwErrorAtLocation(statement.location, "program parameters currently unsupported");
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &)
	{
		Context context;

		// Concatenate all head terms
		std::vector<const Clingo::AST::Term *> headTerms;
		rule.head.data.accept(HeadLiteralCollectFunctionTermsVisitor(), rule.head, headTerms);

		// Print auxiliary variables replacing the head atom’s arguments
		if (!headTerms.empty())
		{
			for (auto i = headTerms.cbegin(); i != headTerms.cend(); i++)
			{
				const auto &headTerm = **i;

				if (i != headTerms.cbegin())
					std::cout << ", ";

				std::cout
					<< AuxiliaryHeadVariablePrefix << (i - headTerms.cbegin())
					<< " in " << headTerm;
			}
		}

		// Print translated body literals
		for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
		{
			const auto &bodyLiteral = *i;

			if (!headTerms.empty())
				std::cout << " and ";

			if (bodyLiteral.sign != Clingo::AST::Sign::None)
				throwErrorAtLocation(bodyLiteral.location, "only positive literals currently supported");

			bodyLiteral.data.accept(BodyLiteralPrintVisitor(), bodyLiteral, context);
		}

		std::cout << " -> ";

		// Print consequent of the implication
		rule.head.data.accept(HeadLiteralPrintSubstitutedVisitor(), rule.head, headTerms);
	}

	void visit(const Clingo::AST::Definition &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("definition", statement);
	}

	void visit(const Clingo::AST::ShowSignature &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("show signature", statement);
	}

	void visit(const Clingo::AST::ShowTerm &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("show term", statement);
	}

	void visit(const Clingo::AST::Minimize &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("minimize", statement);
	}

	void visit(const Clingo::AST::Script &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("script", statement);
	}

	void visit(const Clingo::AST::External &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("external", statement);
	}

	void visit(const Clingo::AST::Edge &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("edge", statement);
	}

	void visit(const Clingo::AST::Heuristic &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("heuristic", statement);
	}

	void visit(const Clingo::AST::ProjectAtom &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("project atom", statement);
	}

	void visit(const Clingo::AST::ProjectSignature &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("project signature", statement);
	}

	void visit(const Clingo::AST::TheoryDefinition &, const Clingo::AST::Statement &statement)
	{
		throwErrorUnsupportedStatement("theory definition", statement);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
