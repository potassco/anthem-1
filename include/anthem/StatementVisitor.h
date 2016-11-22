#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/BodyLiteralVisitor.h>
#include <anthem/HeadLiteralVisitor.h>
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
	const auto errorMessage = std::string("“") + statementType + "” statements currently not supported";

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
			throwErrorAtLocation(statement.location, "program parameters currently not supported");
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &)
	{
		std::vector<Clingo::AST::Term> headTerms;
		rule.head.data.accept(HeadLiteralCollectTermsVisitor(), rule.head, headTerms);

		if (!headTerms.empty())
		{
			std::cout << "exists ";

			for (auto i = headTerms.cbegin(); i != headTerms.cend(); i++)
			{
				if (i != headTerms.cbegin())
					std::cout << ", ";

				std::cout << "AUX" << (i - headTerms.cbegin());
			}

			std::cout << ": ";
		}

		std::cout << "body -> ";

		/*rule.head.data.accept(HeadLiteralVisitor(), rule.head);

		for (const auto &bodyLiteral : rule.body)
		{
			if (bodyLiteral.sign != Clingo::AST::Sign::None)
				throwErrorAtLocation(bodyLiteral.location, "only positive literals currently supported");

			bodyLiteral.data.accept(BodyLiteralVisitor(), bodyLiteral);
		}*/
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
