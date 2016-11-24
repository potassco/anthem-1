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
}
