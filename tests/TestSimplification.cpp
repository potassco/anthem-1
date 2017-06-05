#include <catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[simplification] Rules are simplified correctly", "[simplification]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));
	context.performSimplification = true;
	context.performCompletion = false;

	SECTION("example 1")
	{
		input << ":- in(I, S), in(J, S), in(I + J, S).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((in(U1, U2) and in(U3, U2) and exists X1 (X1 in (U1 + U3) and in(X1, U2))) -> #false)\n");
	}

	SECTION("example 2")
	{
		input << "covered(I) :- in(I, S).";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((V1 = U1 and in(U1, U2)) -> covered(V1))\n");
	}

	SECTION("example 3")
	{
		input << ":- not covered(I), I = 1..n.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "((not covered(U1) and U1 in 1..n) -> #false)\n");
	}

	SECTION("comparisons")
	{
		input << ":- M > N.";
		anthem::translate("input", input, context);

		CHECK(output.str() == "(U1 > U2 -> #false)\n");
	}
}
