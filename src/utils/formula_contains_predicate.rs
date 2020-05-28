pub(crate) fn formula_contains_predicate(formula: &crate::Formula,
	predicate_declaration: &crate::PredicateDeclaration)
	-> bool
{
	use crate::Formula;

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
			arguments.iter().any(
				|argument| formula_contains_predicate(argument, predicate_declaration)),
		Formula::Boolean(_)
		| Formula::Compare(_) => false,
		Formula::Exists(quantified_expression)
		| Formula::ForAll(quantified_expression) =>
			formula_contains_predicate(&quantified_expression.argument, predicate_declaration),
		Formula::Implies(implies) =>
			formula_contains_predicate(&implies.antecedent, predicate_declaration)
			|| formula_contains_predicate(&implies.implication, predicate_declaration),
		Formula::Not(argument) => formula_contains_predicate(argument, predicate_declaration),
		Formula::Predicate(predicate) => &*predicate.declaration == predicate_declaration,
	}
}
