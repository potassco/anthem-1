#include <catch.hpp>

#include <sstream>

#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[translation] Rules are translated correctly", "[translation]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context = {logger, {}, 1};

	SECTION("simple example 1")
	{
		input << "p(1..5).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in (1..5) -> p(V1)\n");
	}

	SECTION("simple example 2")
	{
		input << "p(N) :- N = 1..5.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in N and exists X1, X2 (X1 in N and X2 in (1..5) and X1 = X2) -> p(V1)\n");
	}

	SECTION("simple example 3")
	{
		input << "p(N + 1) :- q(N).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in (N + 1) and exists X1 (X1 in N and q(X1)) -> p(V1)\n");
	}

	SECTION("n-ary head")
	{
		input << "p(N, 1, 2) :- N = 1..5.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in N and V2 in 1 and V3 in 2 and exists X1, X2 (X1 in N and X2 in (1..5) and X1 = X2) -> p(V1, V2, V3)\n");
	}

	SECTION("disjunctive head")
	{
		// TODO: check why order of disjunctive literals is inverted
		input << "q(3, N); p(N, 1, 2) :- N = 1..5.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in N and V2 in 1 and V3 in 2 and V4 in 3 and V5 in N and exists X1, X2 (X1 in N and X2 in (1..5) and X1 = X2) -> p(V1, V2, V3) or q(V4, V5)\n");
	}

	SECTION("disjunctive head (alternative syntax)")
	{
		// TODO: check why order of disjunctive literals is inverted
		input << "q(3, N), p(N, 1, 2) :- N = 1..5.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in N and V2 in 1 and V3 in 2 and V4 in 3 and V5 in N and exists X1, X2 (X1 in N and X2 in (1..5) and X1 = X2) -> p(V1, V2, V3) or q(V4, V5)\n");
	}

	SECTION("escaping conflicting variable names")
	{
		input << "p(X1, V1) :- q(X1), q(V1).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in _X1 and V2 in _V1 and exists X1 (X1 in _X1 and q(X1)) and exists X2 (X2 in _V1 and q(X2)) -> p(V1, V2)\n");
	}

	SECTION("fact")
	{
		input << "p(42).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in 42 -> p(V1)\n");
	}

	SECTION("0-ary fact")
	{
		input << "p.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "#true -> p\n");
	}

	SECTION("disjunctive fact (no arguments)")
	{
		input << "q; p.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "#true -> p or q\n");
	}

	SECTION("disjunctive fact (arguments)")
	{
		input << "q; p(42).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in 42 -> p(V1) or q\n");
	}

	SECTION("integrity constraint (no arguments)")
	{
		input << ":- p, q.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "p and q -> #false\n");
	}

	SECTION("contradiction")
	{
		input << ":-.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "#true -> #false\n");
	}

	SECTION("integrity constraint (arguments)")
	{
		input << ":- p(42), q.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "exists X1 (X1 in 42 and p(X1)) and q -> #false\n");
	}

	SECTION("inf/sup")
	{
		input << "p(X, #inf) :- q(X, #sup).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in #inf and exists X1, X2 (X1 in X and X2 in #sup and q(X1, X2)) -> p(V1, V2)\n");
	}

	SECTION("strings")
	{
		input << "p(X, \"foo\") :- q(X, \"bar\").";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in \"foo\" and exists X1, X2 (X1 in X and X2 in \"bar\" and q(X1, X2)) -> p(V1, V2)\n");
	}

	SECTION("tuples")
	{
		input << "p(X, (1, 2, 3)) :- q(X, (4, 5)).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in (1, 2, 3) and exists X1, X2 (X1 in X and X2 in (4, 5) and q(X1, X2)) -> p(V1, V2)\n");
	}

	SECTION("1-ary tuples")
	{
		input << "p(X, (1,)) :- q(X, (2,)).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in (1,) and exists X1, X2 (X1 in X and X2 in (2,) and q(X1, X2)) -> p(V1, V2)\n");
	}

	SECTION("intervals")
	{
		input << "p(X, 1..10) :- q(X, 6..12).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in (1..10) and exists X1, X2 (X1 in X and X2 in (6..12) and q(X1, X2)) -> p(V1, V2)\n");
	}

	SECTION("comparisons")
	{
		input << "p(M, N, O, P) :- M < N, P != O.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in M and V2 in N and V3 in O and V4 in P and exists X1, X2 (X1 in M and X2 in N and X1 < X2) and exists X3, X4 (X3 in P and X4 in O and X3 != X4) -> p(V1, V2, V3, V4)\n");
	}

	SECTION("single negation")
	{
		input << "not p(X, 1) :- not q(X, 2).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in 1 and exists X1, X2 (X1 in X and X2 in 2 and not q(X1, X2)) -> not p(V1, V2)\n");
	}

	SECTION("variable numbering")
	{
		// TODO: check why order of disjunctive literals is inverted
		input << "f; q(A1, A2); p(A3, r(A4)); g(g(A5)) :- g(A3), f, q(A4, A1), p(A2, A5).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in A1 and V2 in A2 and V3 in A3 and V4 in r(A4) and V5 in g(A5)"
		        " and exists X1 (X1 in A3 and g(X1)) and f and exists X2, X3 (X2 in A4 and X3 in A1 and q(X2, X3)) and exists X4, X5 (X4 in A2 and X5 in A5 and p(X4, X5))"
		        " -> q(V1, V2) or p(V3, V4) or g(V5) or f\n");
	}

	SECTION("nested functions")
	{
		input << "p(q(s(t(X1))), u(X2)) :- u(v(w(X2)), z(X1)).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in q(s(t(_X1))) and V2 in u(_X2) and exists X1, X2 (X1 in v(w(_X2)) and X2 in z(_X1) and u(X1, X2)) -> p(V1, V2)\n");
	}

	SECTION("choice rule (simple)")
	{
		input << "{p}.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "p -> p\n");
	}

	SECTION("choice rule (two elements)")
	{
		input << "{p; q}.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "p or q -> p or q\n");
	}

	SECTION("choice rule (n-ary elements)")
	{
		input << "{p(1..3, N); q(2..4)}.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in (1..3) and V2 in N and V3 in (2..4) and (p(V1, V2) or q(V3)) -> p(V1, V2) or q(V3)\n");
	}
}
