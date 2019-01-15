#include <catch2/catch.hpp>

#include <sstream>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("[integer detection] Integer variables are correctly detected", "[integer detection]")
{
	std::stringstream input;
	std::stringstream output;
	std::stringstream errors;

	anthem::output::Logger logger(output, errors);
	anthem::Context context(std::move(logger));
	context.translationMode = anthem::TranslationMode::Completion;
	context.performSimplification = true;
	context.performCompletion = true;
	context.performIntegerDetection = true;

	SECTION("simple-to-detect integer parameter")
	{
		input << "p(X) :- X = 1..5.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 in (1..5))\n"
			"int(p/1@1)\n");
	}

	SECTION("simple program variable as parameter")
	{
		input <<
			"p(X) :- X = 1..5.\n"
			"p(X) :- X = error.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall V1 (p(V1) <-> (V1 in (1..5) or V1 = error))\n");
	}

	SECTION("integer parameter with arithmetics")
	{
		input << "p(X) :- X = (2 + (1..5)) * 2.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 in ((2 + (1..5)) * 2))\n"
			"int(p/1@1)\n");
	}

	SECTION("integer parameter with arithmetics depending on another integer parameter")
	{
		input
			<< "p(X) :- X = 1..5."
			<< "q(X) :- p(Y), X = (Y + 5) / 3.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 in (1..5))\n"
			"forall N2 (q(N2) <-> exists N3 (p(N3) and N2 in ((N3 + 5) / 3)))\n"
			"int(p/1@1)\n"
			"int(q/1@1)\n");
	}

	SECTION("multiple mixed parameters")
	{
		input
			<< "p(X) :- X = 1..5."
			<< "q(X) :- X = error."
			<< "r(A, B, C) :- p(X), A = X ** 2, q(B), p(C).";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 in (1..5))\n"
			"forall V1 (q(V1) <-> V1 = error)\n"
			"forall N2, V2, N3 (r(N2, V2, N3) <-> exists N4 (p(N4) and N2 = (N4 ** 2) and q(V2) and p(N3)))\n"
			"int(p/1@1)\n"
			"int(r/3@1)\n"
			"int(r/3@3)\n");
	}

	SECTION("integer parameter despite usage of constant symbol")
	{
		input
			<< "p(X) :- X = 2..n.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 in (2..n))\n"
			"int(p/1@1)\n");
	}

	SECTION("integer arithmetics are correctly simplified for operators other than division")
	{
		input
			<< "p(X) :- X = 5 + 9 ** 2.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 = (5 + (9 ** 2)))\n"
			"int(p/1@1)\n");
	}

	SECTION("integer arithmetics are not simplified with the division operator")
	{
		input
			<< "p(X) :- X = 5 + 9 / 0.";
		anthem::translate("input", input, context);

		CHECK(output.str() ==
			"forall N1 (p(N1) <-> N1 in (5 + (9 / 0)))\n"
			"int(p/1@1)\n");
	}
}
