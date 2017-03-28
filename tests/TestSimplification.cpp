#include <catch.hpp>

#include <sstream>

#include <anthem/Context.h>
#include <anthem/Translation.h>
#include <anthem/Simplification.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[simplification] Rules are simplified correctly", "[simplified]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context = {logger, {}};
	context.simplify = true;

	SECTION("example 1")
	{
		input << ":- in(I, S), in(J, S), in(I + J, S).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "((in(I, S) and in(J, S) and exists X5 (X5 in (I + J) and in(X5, S))) -> #false)\n");
	}

	SECTION("example 2")
	{
		input << "covered(I) :- in(I, S).";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "((V1 = I and in(I, S)) -> covered(V1))\n");
	}

	SECTION("example 3")
	{
		input << ":- not covered(I), I = 1..n.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "((not covered(I) and I in 1..n) -> #false)\n");
	}

	SECTION("comparisons")
	{
		input << ":- M > N.";
		anthem::translate("input", input, context);

		REQUIRE(output.str() == "(M > N -> #false)\n");
	}
}
