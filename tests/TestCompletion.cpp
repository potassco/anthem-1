#include <catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[completion] Rules are completed", "[completion]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));
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
			"forall V2 not s(V2)\n"
			"not t\n"
			"forall V3 not u(V3)\n"
			"not q\n"
			"not r(5)\n"
			"forall U1 not s(U1)\n"
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
			"forall V2, V3 not t(V2, V3)\n"
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
			"forall V1 (covered(V1) <-> exists U1 in(V1, U1))\n"
			"forall V2, V3 (in(V2, V3) <-> (V2 in 1..n and V3 in 1..r and in(V2, V3)))\n"
			"forall U2 not (U2 in 1..n and not covered(U2))\n"
			"forall U3, U4, U5 not (in(U3, U4) and in(U5, U4) and exists X1 (X1 in (U3 + U5) and in(X1, U4)))\n");
	}
}
