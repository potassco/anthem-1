#include <catch.hpp>

#include <sstream>

#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[completion] Rules are completed", "[completion]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context = {logger, {}};
	context.simplify = true;
	context.complete = true;

	SECTION("predicate in single rule head")
	{
		input << "p :- q.";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		CHECK(output.str() == "(p <-> q)\n");
	}

	SECTION("predicate in multiple rule heads")
	{
		input << "p :- q.\n"
			"p :- r.\n"
			"p :- s.";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		CHECK(output.str() == "(p <-> (q or r or s))\n");
	}

	SECTION("multiple predicates are correctly separated")
	{
		input << "p :- s.\n"
			"q :- t.\n"
			"p :- q.\n"
			"r :- t.\n"
			"q :- r.";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		CHECK(output.str() == "(p <-> (s or q))\n(q <-> (t or r))\n(r <-> t)\n");
	}

	SECTION("integrity constraints")
	{
		input << ":- q.\n"
			":- s(5).\n"
			"#false :- t.\n"
			"#false :- v(5).";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		CHECK(output.str() == "not q\nnot s(5)\nnot t\nnot v(5)\n");
	}

	SECTION("facts")
	{
		input << "q.\n"
			"r.\n"
			"t :- #true.\n"
			"s :- #true.";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		CHECK(output.str() == "q\nr\nt\ns\n");
	}

	SECTION("useless implications")
	{
		input << "#true :- p, q(N), t(1, 2).\n"
			"#true.\n"
			"h :- #false.";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		// TODO: implement completion for unused predicates
		CHECK(output.str() == "not h\n");
	}

	SECTION("example")
	{
		input << "{in(1..n, 1..r)}.\n"
			"covered(I) :- in(I, S).\n"
			":- I = 1..n, not covered(I).\n"
			":- in(I, S), in(J, S), in(I + J, S).";
		REQUIRE_NOTHROW(anthem::translate("input", input, context));

		CHECK(output.str() == "forall V1, V2 (in(V1, V2) <-> (V1 in 1..n and V2 in 1..r and in(V1, V2)))\n"
			"forall V1 (covered(V1) <-> exists I, S (V1 = I and in(I, S)))\n"
			"forall I not (I in 1..n and not covered(I))\n"
			"forall I, S, J not (in(I, S) and in(J, S) and exists X5 (X5 in (I + J) and in(X5, S)))\n");
	}

	// TODO: test collecting free variables
}
