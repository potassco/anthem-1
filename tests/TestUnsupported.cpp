#include <catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[unsupported] Errors are correctly issued when using unsupported features", "[unsupported]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));

	SECTION("rules with disjunctive head are unsupported")
	{
		context.performCompletion = true;

		input << "a; b.";

		CHECK_THROWS(anthem::translate("input", input, context));
	}

	SECTION("rules with disjunctive head containing elements with arguments are unsupported")
	{
		context.performCompletion = true;

		input << "p(a); p(b).";

		CHECK_THROWS(anthem::translate("input", input, context));
	}

	SECTION("singleton choice rules are supported")
	{
		context.performCompletion = true;

		input << "{a}.";

		CHECK_NOTHROW(anthem::translate("input", input, context));
	}

	SECTION("singleton choice rules containing an element with arguments are supported")
	{
		context.performCompletion = true;

		input << "{p(a)}.";

		CHECK_NOTHROW(anthem::translate("input", input, context));
	}

	SECTION("choice rules with multiple simple elements are supported")
	{
		context.performCompletion = true;

		input << "{a; b}.";

		CHECK_NOTHROW(anthem::translate("input", input, context));
	}

	SECTION("choice rules with multiple elements with arguments are unsupported")
	{
		context.performCompletion = true;

		input << "{p(a); p(b)}.";

		CHECK_THROWS(anthem::translate("input", input, context));
	}
}
