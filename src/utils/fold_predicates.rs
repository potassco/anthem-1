pub(crate) fn fold_predicates<B, F>(formula: &crate::Formula, mut accumulator: B, functor: &mut F)
	-> B
where
	F: FnMut(B, &crate::Predicate) -> B,
{
	use crate::Formula;

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
		{
			for argument in arguments
			{
				accumulator = fold_predicates(argument, accumulator, functor);
			}

			accumulator
		},
		Formula::Boolean(_)
		| Formula::Compare(_) => accumulator,
		Formula::Exists(quantified_expression)
		| Formula::ForAll(quantified_expression) =>
			fold_predicates(&quantified_expression.argument, accumulator, functor),
		Formula::Implies(implies) =>
		{
			accumulator = fold_predicates(&implies.antecedent, accumulator, functor);
			accumulator = fold_predicates(&implies.implication, accumulator, functor);

			accumulator
		},
		Formula::Not(argument) => fold_predicates(argument, accumulator, functor),
		Formula::Predicate(predicate) => functor(accumulator, &predicate),
	}
}
