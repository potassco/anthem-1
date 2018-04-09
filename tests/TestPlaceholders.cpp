#include <catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[placeholders] Programs with placeholders are correctly completed", "[placeholders]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));
	context.performSimplification = true;
	context.performCompletion = true;

	SECTION("no placeholders")
	{
		input <<
			"colored(V, red) :- vertex(V), not colored(V, green), not colored(V, blue).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1, V2 (colored(V1, V2) <-> (V2 = red and vertex(V1) and not colored(V1, green) and not colored(V1, blue)))\n"
			"forall V3 not vertex(V3)\n");
	}

	SECTION("single placeholder")
	{
		input <<
			"#external vertex(1).\n"
			"colored(V, red) :- vertex(V), not colored(V, green), not colored(V, blue).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1, V2 (colored(V1, V2) <-> (V2 = red and vertex(V1) and not colored(V1, green) and not colored(V1, blue)))\n");
	}

	SECTION("complex example: graph coloring")
	{
		input <<
			"#external color(1).\n"
			"#external edge(2).\n"
			"#external vertex(1).\n"
			"#show color/2.\n"
			"{color(V, C)} :- vertex(V), color(C).\n"
			"covered(V) :- color(V, _).\n"
			":- vertex(V), not covered(V).\n"
			":- color(V1, C), color(V2, C), edge(V1, V2).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1, V2 (color(V1, V2) -> (vertex(V1) and color(V2)))\n"
			"forall U1 (vertex(U1) -> exists U2 color(U1, U2))\n"
			"forall U3, U4, U5 (not color(U3, U4) or not color(U5, U4) or not edge(U3, U5))\n");
	}
}
