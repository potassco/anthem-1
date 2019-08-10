#include <anthem/verify-properties/Translation.h>

#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>
#include <anthem/translation-common/Output.h>
#include <anthem/verify-properties/Output.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(std::vector<ast::ScopedFormula> &&scopedFormulas, Context &context)
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}
