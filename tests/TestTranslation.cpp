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
	anthem::Context context = {logger};

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

	SECTION("integrity constraint")
	{
		input << ":- p(42).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "exists X1 (X1 in 42 and p(X1)) -> #false\n");
	}

	SECTION("inf/sup")
	{
		input << "p(X, #inf) :- q(X, #sup).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "V1 in X and V2 in #inf and exists X1, X2 (X1 in X and X2 in #sup and q(X1, X2)) -> p(V1, V2)\n");
	}
}
