#include <catch2/catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[translation] Rules are translated correctly", "[translation]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));
	context.performSimplification = false;
	context.performCompletion = false;

	SECTION("simple example 1")
	{
		input << "p(1..5).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(V1 in (1..5) -> p(V1))\n");
	}

	SECTION("simple example 2")
	{
		input << "p(N) :- N = 1..5.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and exists X1, X2 (X1 in U1 and X2 in (1..5) and X1 = X2)) -> p(V1))\n");
	}

	SECTION("simple example 3")
	{
		input << "p(N + 1) :- q(N).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in (U1 + 1) and exists X1 (X1 in U1 and q(X1))) -> p(V1))\n");
	}

	SECTION("n-ary head")
	{
		input << "p(N, 1, 2) :- N = 1..5.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in 1 and V3 in 2 and exists X1, X2 (X1 in U1 and X2 in (1..5) and X1 = X2)) -> p(V1, V2, V3))\n");
	}

	SECTION("disjunctive head")
	{
		// TODO: check why order of disjunctive literals is inverted
		input << "q(3, N); p(N, 1, 2) :- N = 1..5.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in 1 and V3 in 2 and V4 in 3 and V5 in U1 and exists X1, X2 (X1 in U1 and X2 in (1..5) and X1 = X2)) -> (p(V1, V2, V3) or q(V4, V5)))\n");
	}

	SECTION("disjunctive head (alternative syntax)")
	{
		// TODO: check why order of disjunctive literals is inverted
		input << "q(3, N), p(N, 1, 2) :- N = 1..5.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in 1 and V3 in 2 and V4 in 3 and V5 in U1 and exists X1, X2 (X1 in U1 and X2 in (1..5) and X1 = X2)) -> (p(V1, V2, V3) or q(V4, V5)))\n");
	}

	SECTION("escaping conflicting variable names")
	{
		input << "p(X1, V1, A1) :- q(X1), q(V1), q(A1).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in U2 and V3 in U3 and exists X1 (X1 in U1 and q(X1)) and exists X2 (X2 in U2 and q(X2)) and exists X3 (X3 in U3 and q(X3))) -> p(V1, V2, V3))\n");
	}

	SECTION("fact")
	{
		input << "p(42).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(V1 in 42 -> p(V1))\n");
	}

	SECTION("0-ary fact")
	{
		input << "p.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(#true -> p)\n");
	}

	SECTION("function")
	{
		input << ":- not p(I), I = 1..n.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((exists X1 (X1 in U1 and not p(X1)) and exists X2, X3 (X2 in U1 and X3 in (1..n) and X2 = X3)) -> #false)\n");
	}

	SECTION("disjunctive fact (no arguments)")
	{
		input << "q; p.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(#true -> (p or q))\n");
	}

	SECTION("disjunctive fact (arguments)")
	{
		input << "q; p(42).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(V1 in 42 -> (p(V1) or q))\n");
	}

	SECTION("integrity constraint (no arguments)")
	{
		input << ":- p, q.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((p and q) -> #false)\n");
	}

	SECTION("contradiction")
	{
		input << ":-.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(#true -> #false)\n");
	}

	SECTION("integrity constraint (arguments)")
	{
		input << ":- p(42), q.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((exists X1 (X1 in 42 and p(X1)) and q) -> #false)\n");
	}

	SECTION("inf/sup")
	{
		input << "p(X, #inf) :- q(X, #sup).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in #inf and exists X1, X2 (X1 in U1 and X2 in #sup and q(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("strings")
	{
		input << "p(X, \"foo\") :- q(X, \"bar\").";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in \"foo\" and exists X1, X2 (X1 in U1 and X2 in \"bar\" and q(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("tuples")
	{
		input << "p(X, (1, 2, 3)) :- q(X, (4, 5)).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in (1, 2, 3) and exists X1, X2 (X1 in U1 and X2 in (4, 5) and q(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("1-ary tuples")
	{
		input << "p(X, (1,)) :- q(X, (2,)).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in (1,) and exists X1, X2 (X1 in U1 and X2 in (2,) and q(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("intervals")
	{
		input << "p(X, 1..10) :- q(X, 6..12).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in (1..10) and exists X1, X2 (X1 in U1 and X2 in (6..12) and q(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("intervals with variable")
	{
		input << ":- q(N), 1 = 1..N.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((exists X1 (X1 in U1 and q(X1)) and exists X2, X3 (X2 in 1 and X3 in (1..U1) and X2 = X3)) -> #false)\n");
	}

	SECTION("intervals with two variables")
	{
		input << ":- q(M, N), M = 1..N.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((exists X1, X2 (X1 in U1 and X2 in U2 and q(X1, X2)) and exists X3, X4 (X3 in U1 and X4 in (1..U2) and X3 = X4)) -> #false)\n");
	}

	SECTION("comparisons")
	{
		input << "p(M, N, O, P) :- M < N, P != O.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in U2 and V3 in U3 and V4 in U4 and exists X1, X2 (X1 in U1 and X2 in U2 and X1 < X2) and exists X3, X4 (X3 in U4 and X4 in U3 and X3 != X4)) -> p(V1, V2, V3, V4))\n");
	}

	SECTION("single negation with 0-ary predicates")
	{
		input << "not p :- not q.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(not q -> not p)\n");
	}

	SECTION("single negation with n-ary predicates")
	{
		input << "not p(X, 1) :- not q(X, 2).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in 1 and exists X1, X2 (X1 in U1 and X2 in 2 and not q(X1, X2))) -> not p(V1, V2))\n");
	}

	SECTION("variable numbering")
	{
		// TODO: check why order of disjunctive literals is inverted
		input << "f; q(A1, A2); p(A3, r(A4)); g(g(A5)) :- g(A3), f, q(A4, A1), p(A2, A5).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in U2 and V3 in U3 and V4 in r(U4) and V5 in g(U5)"
		        " and exists X1 (X1 in U3 and g(X1)) and f and exists X2, X3 (X2 in U4 and X3 in U1 and q(X2, X3)) and exists X4, X5 (X4 in U2 and X5 in U5 and p(X4, X5)))"
		        " -> (q(V1, V2) or p(V3, V4) or g(V5) or f))\n");
	}

	SECTION("nested functions")
	{
		input << "p(q(s(t(X1))), u(X2)) :- u(v(w(X2)), z(X1)).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in q(s(t(U1))) and V2 in u(U2) and exists X1, X2 (X1 in v(w(U2)) and X2 in z(U1) and u(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("choice rule (simple)")
	{
		input << "{p}.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(p -> p)\n");
	}

	SECTION("choice rule (two elements)")
	{
		input << "{p; q}.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(p -> p)\n(q -> q)\n");
	}

	SECTION("choice rule (n-ary elements)")
	{
		input << "{p(1..3, N); q(2..4)}.";
		anthem::translate("input", input, context);

		// TODO: eliminate V5: not needed
		CHECK(output.str() == "((V1 in (1..3) and V2 in U1 and V3 in (2..4) and p(V1, V2)) -> p(V1, V2))\n((V4 in (1..3) and V5 in U2 and V6 in (2..4) and q(V6)) -> q(V6))\n");
	}

	SECTION("choice rule with body")
	{
		input << "{p(M, N); q(P)} :- s(M, N, P).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in U2 and V3 in U3 and exists X1, X2, X3 (X1 in U1 and X2 in U2 and X3 in U3 and s(X1, X2, X3)) and p(V1, V2)) -> p(V1, V2))\n((V4 in U4 and V5 in U5 and V6 in U6 and exists X4, X5, X6 (X4 in U4 and X5 in U5 and X6 in U6 and s(X4, X5, X6)) and q(V6)) -> q(V6))\n");
	}

	SECTION("choice rule with negation")
	{
		input << "{not p(X, 1)} :- not q(X, 2).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in 1 and exists X1, X2 (X1 in U1 and X2 in 2 and not q(X1, X2)) and not p(V1, V2)) -> not p(V1, V2))\n");
	}

	SECTION("choice rule with negation (two elements)")
	{
		input << "{not p(X, 1); not s} :- not q(X, 2).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in 1 and exists X1, X2 (X1 in U1 and X2 in 2 and not q(X1, X2)) and not p(V1, V2)) -> not p(V1, V2))\n((V3 in U2 and V4 in 1 and exists X3, X4 (X3 in U2 and X4 in 2 and not q(X3, X4)) and not s) -> not s)\n");
	}

	SECTION("anonymous variables")
	{
		input << "p(_, _) :- q(_, _).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in U2 and exists X1, X2 (X1 in U3 and X2 in U4 and q(X1, X2))) -> p(V1, V2))\n");
	}

	SECTION("exponentiation operator")
	{
		input << "p(N, N ** N) :- N = 1..n.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 in U1 and V2 in (U1 ** U1) and exists X1, X2 (X1 in U1 and X2 in (1..n) and X1 = X2)) -> p(V1, V2))\n");
	}

	SECTION("unary minus")
	{
		input << "p(-5).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(V1 in -5 -> p(V1))\n");
	}

	SECTION("unary minus in interval")
	{
		input << "p(-5..5).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(V1 in (-5..5) -> p(V1))\n");
	}
}
