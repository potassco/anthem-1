#include <anthem/verify-properties/Translation.h>

#include <clingo.hh>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/RuleContext.h>
#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>
#include <anthem/translation-common/ChooseValueInTerm.h>
#include <anthem/translation-common/Output.h>
#include <anthem/verify-properties/Body.h>
#include <anthem/verify-properties/Head.h>
#include <anthem/verify-properties/TranslationContext.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

inline void read(const Clingo::AST::Rule &rule, Context &context, TranslationContext &translationContext)
{
	RuleContext ruleContext;
	ast::VariableStack variableStack;
	variableStack.push(&ruleContext.freeVariables);

	ast::And translatedBody;

	// Translate body literals
	for (const auto &bodyLiteral : rule.body)
	{
		auto argument = bodyLiteral.data.accept(BodyBodyLiteralVisitor(), bodyLiteral, context, ruleContext, variableStack);
		translatedBody.arguments.emplace_back(std::move(argument));
	}

	const auto headTranslationResult = rule.head.data.accept(HeadLiteralVisitor(), rule.head, context);

	const auto translateHeadTerms =
		[&](const HeadAtom &headAtom, ast::And &formula)
		{
			for (const auto &argument : headAtom.arguments)
			{
				auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Head);

				auto translatedHeadTerm = translationCommon::chooseValueInTerm(argument, *variableDeclaration, context, ruleContext, variableStack);
				formula.arguments.emplace_back(std::move(translatedHeadTerm));

				ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));
			}
		};

	switch (headTranslationResult.headType)
	{
		case HeadType::SingleAtom:
		{
			assert(headTranslationResult.headAtom);
			const auto &headAtom = *headTranslationResult.headAtom;

			auto formula = std::move(translatedBody);
			translateHeadTerms(headAtom, formula);

			ast::ScopedFormula scopedFormula{std::move(formula), std::move(ruleContext.freeVariables)};

			translationContext.definitions[headAtom.predicate.declaration].emplace_back(std::move(scopedFormula));
		}
		default:
			// TODO: implement
			return;
	}

	throw LogicException("unreachable code, please report to bug tracker");
}

void translate(const std::vector<std::string> &fileNames, Context &context)
{
}

void translate(const char *fileName, std::istream &stream, Context &context)
{
}

/*void translate(Context &context, TranslationContext &&translationContext)
{
	TranslationContext translationContext;

	output::PrintContext printContext(context);
	auto &stream = context.logger.outputStream();

	const auto buildUniversallyClosedFormulas =
		[](std::vector<ast::ScopedFormula> &&scopedFormulas)
		{
			std::vector<ast::Formula> universallyClosedFormulas;
			universallyClosedFormulas.reserve(scopedFormulas.size());

			// Build the universal closure
			for (auto &scopedFormula : scopedFormulas)
			{
				const auto makeUniversallyClosedFormula =
					[&]() -> ast::Formula
					{
						if (scopedFormula.freeVariables.empty())
							return std::move(scopedFormula.formula);

						return ast::ForAll(std::move(scopedFormula.freeVariables), std::move(scopedFormula.formula));
					};

				universallyClosedFormulas.emplace_back(makeUniversallyClosedFormula());
			}

			return universallyClosedFormulas;
		};

	const auto buildFinalFormulas =
		[&]()
		{
			// If we’re just given one program, translate it to individual axioms
			if (!scopedFormulasB)
				return buildUniversallyClosedFormulas(std::move(scopedFormulasA));

			// If we’re given two programs A and B, translate them to a conjecture of the form “A <=> B”
			auto universallyClosedFormulasA = buildUniversallyClosedFormulas(std::move(scopedFormulasA));
			auto universallyClosedFormulasB = buildUniversallyClosedFormulas(std::move(scopedFormulasB.value()));

			// Build the conjunctions of all formulas resulting from each program respectively
			ast::And conjunctionA(std::move(universallyClosedFormulasA));
			ast::And conjunctionB(std::move(universallyClosedFormulasB));

			std::vector<ast::Formula> finalFormulas;
			finalFormulas.reserve(1);
			finalFormulas.emplace_back(ast::Biconditional(std::move(conjunctionA), std::move(conjunctionB)));

			return finalFormulas;
		};

	auto finalFormulas = buildFinalFormulas();

	const auto performDomainMapping =
		[&]()
		{
			switch (context.mapToIntegersPolicy)
			{
				case MapToIntegersPolicy::Always:
					return true;
				case MapToIntegersPolicy::Auto:
					return (context.outputFormat == OutputFormat::TPTP);
			}

			throw TranslationException("supposedly unreachable code, please report to the bug tracker");
		};

	// If requested, map both program and integer variables to integers
	if (performDomainMapping())
		for (auto &finalFormula : finalFormulas)
			mapDomains(finalFormula, context);

	// Print auxiliary definitions for mapping program and integer variables to even and odd integers
	if (context.outputFormat == OutputFormat::TPTP)
	{
		stream
			<< R"(%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  types
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(types, type, object: $tType).
)";
	}

	// Print type annotations for predicate signatures
	for (const auto &predicateDeclaration : context.predicateDeclarations)
		translationCommon::printTypeAnnotation(*predicateDeclaration, context, printContext);

	// Print type annotations for function signatures
	for (const auto &functionDeclaration : context.functionDeclarations)
		translationCommon::printTypeAnnotation(*functionDeclaration, context, printContext);

	if (context.outputFormat == OutputFormat::TPTP)
	{
		stream
			<< R"(
tff(types, type, (f__integer__: ($int) > object)).
tff(types, type, (f__symbolic__: ($i) > object)).

tff(types, type, (f__sum__: (object * object) > object)).
tff(types, type, (f__unary_minus__: (object) > object)).
tff(types, type, (f__difference__: (object * object) > object)).
tff(types, type, (f__product__: (object * object) > object)).

tff(types, type, (p__is_integer__: (object) > $o)).
tff(types, type, (p__is_symbolic__: (object) > $o)).
tff(types, type, (p__less_equal__: (object * object) > $o)).
tff(types, type, (p__less__: (object * object) > $o)).
tff(types, type, (p__greater_equal__: (object * object) > $o)).
tff(types, type, (p__greater__: (object * object) > $o)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  objects: integers vs. symbolics
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(type_check, axiom, (![X: object]: (p__is_integer__(X) <=> (?[Y: $int]: (X = f__integer__(Y)))))).
tff(type_check, axiom, (![X: object]: (p__is_symbolic__(X) <=> (?[Y: $i]: (X = f__symbolic__(Y)))))).
tff(type_check, axiom, (![X: object]: (p__is_integer__(X) <~> p__is_symbolic__(X)))).
tff(type_check, axiom, (![X: $int, Y: $int]: ((f__integer__(X) = f__integer__(Y)) => (X = Y)))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  integer operations
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(operations, axiom, (![X1: $int, X2: $int]: (f__sum__(f__integer__(X1), f__integer__(X2)) = f__integer__($sum(X1, X2))))).
tff(operations, axiom, (![X: $int]: (f__unary_minus__(f__integer__(X)) = f__integer__($uminus(X))))).
tff(operations, axiom, (![X1: $int, X2: $int]: (f__difference__(f__integer__(X1), f__integer__(X2)) = f__integer__($difference(X1, X2))))).
tff(operations, axiom, (![X1: $int, X2: $int]: (f__product__(f__integer__(X1), f__integer__(X2)) = f__integer__($product(X1, X2))))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  object comparisons
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
tff(less_equal, axiom, (![X1: $int, X2: $int]: (p__less_equal__(f__integer__(X1), f__integer__(X2)) <=> $lesseq(X1, X2)))).
tff(less_equal, axiom, (![X1: $i, X2: $int]: ~p__less_equal__(f__symbolic__(X1), f__integer__(X2)))).
tff(less_equal, axiom, (![X1: $int, X2: $i]: p__less_equal__(f__integer__(X1), f__symbolic__(X2)))).

tff(less, axiom, (![X1: $int, X2: $int]: (p__less__(f__integer__(X1), f__integer__(X2)) <=> $less(X1, X2)))).
tff(less, axiom, (![X1: $i, X2: $int]: ~p__less__(f__symbolic__(X1), f__integer__(X2)))).
tff(less, axiom, (![X1: $int, X2: $i]: p__less__(f__integer__(X1), f__symbolic__(X2)))).

tff(greater_equal, axiom, (![X1: $int, X2: $int]: (p__greater_equal__(f__integer__(X1), f__integer__(X2)) <=> $greatereq(X1, X2)))).
tff(greater_equal, axiom, (![X1: $i, X2: $int]: p__greater_equal__(f__symbolic__(X1), f__integer__(X2)))).
tff(greater_equal, axiom, (![X1: $int, X2: $i]: ~p__greater_equal__(f__integer__(X1), f__symbolic__(X2)))).

tff(greater, axiom, (![X1: $int, X2: $int]: (p__greater__(f__integer__(X1), f__integer__(X2)) <=> $greater(X1, X2)))).
tff(greater, axiom, (![X1: $i, X2: $int]: p__greater__(f__symbolic__(X1), f__integer__(X2)))).
tff(greater, axiom, (![X1: $int, X2: $i]: ~p__greater__(f__integer__(X1), f__symbolic__(X2)))).
)"
			<< std::endl;
	}

	if (scopedFormulasB)
		assert(finalFormulas.size() == 1);

	const auto formulaType =
		[&scopedFormulasB]()
		{
			if (scopedFormulasB)
				return translationCommon::FormulaType::Conjecture;

			return translationCommon::FormulaType::Axiom;
		};

	// Print translated formulas
	for (auto &finalFormula : finalFormulas)
	{
		translationCommon::printFormula(finalFormula, formulaType(), context, printContext);
		context.logger.outputStream() << std::endl;
	}
}*/

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
