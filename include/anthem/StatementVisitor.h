#ifndef __ANTHEM__STATEMENT_VISITOR_H
#define __ANTHEM__STATEMENT_VISITOR_H

#include <anthem/AST.h>
#include <anthem/ASTCopy.h>
#include <anthem/Body.h>
#include <anthem/Head.h>
#include <anthem/RuleContext.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// StatementVisitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Replaces empty and 1-element conjunctions in the antecedent of normal-form formulas
inline void reduce(ast::Implies &implies)
{
	if (!implies.antecedent.is<ast::And>())
		return;

	auto &antecedent = implies.antecedent.get<ast::And>();

	// Use “true” as the consequent in case it is empty
	if (antecedent.arguments.empty())
		implies.antecedent = ast::Formula::make<ast::Boolean>(true);
	else if (antecedent.arguments.size() == 1)
		implies.antecedent = std::move(antecedent.arguments[0]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct StatementVisitor
{
	void visit(const Clingo::AST::Program &program, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		// TODO: refactor
		context.logger.log(output::Priority::Debug, (std::string("[program] ") + program.name).c_str());

		if (!program.parameters.empty())
			throwErrorAtLocation(statement.location, "program parameters currently unsupported", context);
	}

	void visit(const Clingo::AST::Rule &rule, const Clingo::AST::Statement &, std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
	{
		RuleContext ruleContext;
		ast::VariableStack variableStack;
		variableStack.push(&ruleContext.freeVariables);

		// Collect all head terms
		rule.head.data.accept(HeadLiteralCollectFunctionTermsVisitor(), rule.head, context, ruleContext);

		// Create new variable declarations for the head terms
		ruleContext.headVariablesStartIndex = ruleContext.freeVariables.size();
		ruleContext.freeVariables.reserve(ruleContext.headTerms.size());

		for (size_t i = 0; i < ruleContext.headTerms.size(); i++)
		{
			// TODO: drop name
			auto variableName = "#" + std::string(HeadVariablePrefix) + std::to_string(ruleContext.freeVariables.size() + 1);
			auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(std::move(variableName), ast::VariableDeclaration::Type::Head);

			ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));
		}

		ast::And antecedent;

		// Compute consequent
		auto headVariableIndex = ruleContext.headVariablesStartIndex;
		auto consequent = rule.head.data.accept(HeadLiteralTranslateToConsequentVisitor(), rule.head, context, ruleContext, headVariableIndex);

		assert(ruleContext.headTerms.size() == headVariableIndex - ruleContext.headVariablesStartIndex);

		if (!consequent)
		{
			// TODO: think about throwing an exception instead
			context.logger.log(output::Priority::Error, "could not translate formula consequent");
			return;
		}

		// Generate auxiliary variables replacing the head atom’s arguments
		for (auto i = ruleContext.headTerms.cbegin(); i != ruleContext.headTerms.cend(); i++)
		{
			const auto &headTerm = **i;

			const auto auxiliaryHeadVariableID = ruleContext.headVariablesStartIndex + i - ruleContext.headTerms.cbegin();
			auto element = ast::Variable(ruleContext.freeVariables[auxiliaryHeadVariableID].get());
			auto set = translate(headTerm, context, ruleContext, variableStack);
			auto in = ast::In(std::move(element), std::move(set));

			antecedent.arguments.emplace_back(std::move(in));
		}

		// Translate body literals
		for (auto i = rule.body.cbegin(); i != rule.body.cend(); i++)
		{
			const auto &bodyLiteral = *i;

			auto argument = bodyLiteral.data.accept(BodyBodyLiteralTranslateVisitor(), bodyLiteral, context, ruleContext, variableStack);

			if (!argument)
				throwErrorAtLocation(bodyLiteral.location, "could not translate body literal", context);

			antecedent.arguments.emplace_back(std::move(argument.value()));
		}

		if (!ruleContext.isChoiceRule)
		{
			auto formula = ast::Formula::make<ast::Implies>(std::move(antecedent), std::move(consequent.value()));
			ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
			scopedFormulas.emplace_back(std::move(scopedFormula));
			reduce(scopedFormulas.back().formula.get<ast::Implies>());
		}
		else
		{
			const auto createFormula =
				[&](ast::Formula &argument, bool isLastOne)
				{
					auto &consequent = argument;

					if (!isLastOne)
					{
						auto formula = ast::Formula::make<ast::Implies>(ast::prepareCopy(antecedent), std::move(consequent));
						ast::ScopedFormula scopedFormula(std::move(formula), {});
						ast::fixDanglingVariables(scopedFormula);
						scopedFormulas.emplace_back(std::move(scopedFormula));
					}
					else
					{
						auto formula = ast::Formula::make<ast::Implies>(std::move(antecedent), std::move(consequent));
						ast::ScopedFormula scopedFormula(std::move(formula), std::move(ruleContext.freeVariables));
						scopedFormulas.emplace_back(std::move(scopedFormula));
					}

					auto &implies = scopedFormulas.back().formula.get<ast::Implies>();
					auto &antecedent = implies.antecedent.get<ast::And>();
					antecedent.arguments.emplace_back(ast::prepareCopy(implies.consequent));
					ast::fixDanglingVariables(scopedFormulas.back());

					reduce(implies);
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
	}

	template<class T>
	void visit(const T &, const Clingo::AST::Statement &statement, std::vector<ast::ScopedFormula> &, Context &context)
	{
		throwErrorAtLocation(statement.location, "statement currently unsupported, expected rule", context);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
