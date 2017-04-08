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
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "(p <-> q)\n");
	}

	SECTION("predicate in multiple rule heads")
	{
		input << "p :- q.\n"
			"p :- r.\n"
			"p :- s.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "(p <-> (q or r or s))\n");
	}

	SECTION("multiple predicates are correctly separated")
	{
		input << "p :- s.\n"
			"q :- t.\n"
			"p :- q.\n"
			"r :- t.\n"
			"q :- r.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "(p <-> (s or q))\n(q <-> (t or r))\n(r <-> t)\n");
	}
}
