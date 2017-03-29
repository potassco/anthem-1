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

		// Generate auxiliary variables replacing the head atom’s arguments
		for (auto i = context.headTerms.cbegin(); i != context.headTerms.cend(); i++)
		{
			const auto &headTerm = **i;

			auto variableName = std::string(AuxiliaryHeadVariablePrefix) + std::to_string(i - context.headTerms.cbegin() + 1);
			auto element = ast::Variable(std::move(variableName), ast::Variable::Type::Reserved);
			auto set = translate(headTerm, context);
			auto in = ast::In(std::move(element), std::move(set));

			antecedent.arguments.emplace_back(std::move(in));
		}

		// Translate body literals
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

		std::vector<ast::Formula> formulas;

		if (!context.isChoiceRule)
			formulas.emplace_back(ast::Formula::make<ast::Implies>(std::move(antecedent), std::move(consequent.value())));
		else
		{
			const auto createFormula =
				[&](ast::Formula &argument, bool isLastOne)
				{
					auto consequent = std::move(argument);

					if (!isLastOne)
						formulas.emplace_back(ast::Formula::make<ast::Implies>(ast::deepCopy(antecedent), std::move(consequent)));
					else
						formulas.emplace_back(ast::Formula::make<ast::Implies>(std::move(antecedent), std::move(consequent)));

					auto &implies = formulas.back().get<ast::Implies>();
					auto &antecedent = implies.antecedent.get<ast::And>();
					antecedent.arguments.emplace_back(ast::deepCopy(implies.consequent));
				};

			if (consequent.value().is<ast::Or>())
			{
				auto &disjunction = consequent.value().get<ast::Or>();

				for (auto &argument : disjunction.arguments)
					createFormula(argument, &argument == &disjunction.arguments.back());
			}
			// TODO: check whether this is really correct for all possible consequent types
			else
				createFormula(consequent.value(), true);
		}

		for (auto &formula : formulas)
		{
			auto &implies = formula.get<ast::Implies>();
			auto &antecedent = implies.antecedent.get<ast::And>();

			// Use “true” as the consequent in case it is empty
			if (antecedent.arguments.empty())
				implies.antecedent = ast::Formula::make<ast::Boolean>(true);
			else if (antecedent.arguments.size() == 1)
			{
				// TODO: remove workaround
				auto tmp = std::move(antecedent.arguments[0]);
				implies.antecedent = std::move(tmp);
			}
		}

		return formulas;
	}

	template<class T>
	std::vector<ast::Formula> visit(const T &, const Clingo::AST::Statement &statement, Context &context)
	{
		throwErrorAtLocation(statement.location, "statement currently unsupported, expected rule", context);
		return {};
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
