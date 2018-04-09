#include <catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[hidden predicate elimination] Hidden predicates are correctly eliminated", "[hidden predicate elimination]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));
	context.performSimplification = true;
	context.performCompletion = true;

	SECTION("simple example (positive 0-ary predicate)")
	{
		input <<
			"p :- q.\n"
			"q.\n"
			"#show p/0.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"(p <-> #true)\n");
	}

	SECTION("simple example (negative 0-ary predicate)")
	{
		input <<
			"p :- q.\n"
			"#show p/0.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"(p <-> #false)\n");
	}

	SECTION("simple example (1-ary predicates, hide all but one)")
	{
		input <<
			"a(X) :- b(X), c(X).\n"
			"b(X) :- not d(X).\n"
			"c(1).\n"
			"c(2).\n"
			"#show a/1.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1 (a(V1) <-> (not #false and (V1 = 1 or V1 = 2)))\n");
	}

	SECTION("simple example (1-ary predicates, show all explicitly)")
	{
		input <<
			"a(X) :- b(X), c(X).\n"
			"b(X) :- not d(X).\n"
			"c(1).\n"
			"c(2).\n"
			"#show a/1."
			"#show b/1."
			"#show c/1."
			"#show d/1.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1 (a(V1) <-> (b(V1) and c(V1)))\n"
			"forall V2 (b(V2) <-> not d(V2))\n"
			"forall V3 (c(V3) <-> (V3 = 1 or V3 = 2))\n"
			"forall V4 not d(V4)\n");
	}

	SECTION("simple example (1-ary predicates, show all implicitly)")
	{
		input <<
			"a(X) :- b(X), c(X).\n"
			"b(X) :- not d(X).\n"
			"c(1).\n"
			"c(2).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1 (a(V1) <-> (b(V1) and c(V1)))\n"
			"forall V2 (b(V2) <-> not d(V2))\n"
			"forall V3 (c(V3) <-> (V3 = 1 or V3 = 2))\n"
			"forall V4 not d(V4)\n");
	}

	SECTION("circular dependencies cannot be fully removed")
	{
		input <<
			"a(X) :- b(X).\n"
			"b(X) :- not c(X).\n"
			"c(X) :- d(X).\n"
			"d(X) :- not b(X).\n"
			"{e(X)}.\n"
			"#show a/1.";
		anthem::translate("input", input, context);

		// TODO: simplify further
		CHECK(output.str() ==
			"forall V1 (a(V1) <-> not d(V1))\n"
			"forall V2 (d(V2) <-> d(V2))\n"
			"forall V3 (e(V3) <-> e(V3))\n");
	}

	SECTION("simple Booleans are recognized")
	{
		input <<
			"a(X) :- c(X).\n"
			"b(X) :- d(X).\n"
			"c(X).\n"
			"#show a/1.\n"
			"#show b/1.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1 (a(V1) <-> #true)\n"
			"forall V2 (b(V2) <-> #false)\n");
	}

	SECTION("predicate arity is respected")
	{
		input <<
			"a(X) :- a(X, Y).\n"
			"a(X, Y) :- a(X, Y, Z).\n"
			"a(1, 2, 3).\n"
			"a(2, 4, 6).\n"
			"#show a/2.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1, V2 (a(V1, V2) <-> exists U1 ((V1 = 1 and V2 = 2 and U1 = 3) or (V1 = 2 and V2 = 4 and U1 = 6)))\n");
	}

	SECTION("nested arguments are correctly handled when hiding predicates")
	{
		input <<
			"a(X) :- b(c(X)).\n"
			"b(c(X)) :- c(X).\n"
			"c(1..4).\n"
			"#show a/1.";
		anthem::translate("input", input, context);

		// TODO: simplify further
		CHECK(output.str() ==
			"forall V1 (a(V1) <-> exists U1 (c(V1) = c(U1) and U1 in 1..4))\n");
	}

	SECTION("simple propositions are hidden correctly")
	{
		input <<
			"p :- q.\n"
			"q :- not r.\n"
			"{s; t} :- p.\n"
			":- s, not t.\n"
			":- p, q, r.\n"
			"#show s/0.\n"
			"#show t/0.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"(s -> not #false)\n"
			"(t -> not #false)\n"
			"(s -> t)\n"
			"(#false or #false or not #false)\n");
	}

	SECTION("predicate with more than one argument is hidden correctly")
	{
		input <<
			"a(X, Y, Z) :- p(X, Y, Z).\n"
			"p(X, Y, Z).\n"
			"#show a/3.";
		anthem::translate("input", input, context);

		// TODO: simplify further
		CHECK(output.str() ==
			"forall V1, V2, V3 (a(V1, V2, V3) <-> #true)\n");
	}
}
