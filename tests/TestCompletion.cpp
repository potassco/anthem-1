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

	SECTION("predicte in single rule head")
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

	// TODO: test collecting free variables
}
