#include <catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

/*TEST_CASE("[completion] Rules are completed", "[completion]")
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
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"(p <-> q)\n"
			"not q\n");
	}

	SECTION("predicate in multiple rule heads")
	{
		input <<
			"p :- q.\n"
			"p :- r.\n"
			"p :- s.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"(p <-> (q or r or s))\n"
			"not q\n"
			"not r\n"
			"not s\n");
	}

	SECTION("multiple predicates are correctly separated")
	{
		input <<
			"p :- s.\n"
			"q :- t.\n"
			"p :- q.\n"
			"r :- t.\n"
			"q :- r.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"(p <-> (s or q))\n"
			"(q <-> (t or r))\n"
			"(r <-> t)\n"
			"not s\n"
			"not t\n");
	}

	SECTION("integrity constraints")
	{
		input <<
			":- q.\n"
			":- r(5).\n"
			":- s(N).\n"
			"#false :- t.\n"
			"#false :- u(5).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"not q\n"
			"forall V1 not r(V1)\n"
			"forall V1 not s(V1)\n"
			"not t\n"
			"forall V1 not u(V1)\n"
			"not q\n"
			"not r(5)\n"
			"forall N not s(N)\n"
			"not t\n"
			"not u(5)\n");
	}

	SECTION("Booleans")
	{
		input <<
			"#true :- #true.\n"
			"#true :- #false.\n"
			"#false :- #true.\n"
			"#false :- #false.\n";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"not #true\n"
			"not #false\n");
	}

	SECTION("facts")
	{
		input <<
			"q.\n"
			"r.\n"
			"s :- #true.\n"
			"t :- #true.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"q\n"
			"r\n"
			"s\n"
			"t\n");
	}

	SECTION("useless implications")
	{
		input <<
			"#true :- p, q(N), t(1, 2).\n"
			"#true.\n"
			"v :- #false.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"not p\n"
			"forall V1 not q(V1)\n"
			"forall V1, V2 not t(V1, V2)\n"
			"not v\n");
	}

	SECTION("example")
	{
		input <<
			"{in(1..n, 1..r)}.\n"
			"covered(I) :- in(I, S).\n"
			":- I = 1..n, not covered(I).\n"
			":- in(I, S), in(J, S), in(I + J, S).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1 (covered(V1) <-> exists I, S (V1 = I and in(I, S)))\n"
			"forall V1, V2 (in(V1, V2) <-> (V1 in 1..n and V2 in 1..r and in(V1, V2)))\n"
			"forall I not (I in 1..n and not covered(I))\n"
			"forall I, S, J not (in(I, S) and in(J, S) and exists X5 (X5 in (I + J) and in(X5, S)))\n");
	}

	// TODO: test collecting free variables
}
*/
