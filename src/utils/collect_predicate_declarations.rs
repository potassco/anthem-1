fn collect_predicate_declarations_in_formula(formula: &crate::Formula,
	mut predicate_declarations:
		&mut std::collections::BTreeSet<std::rc::Rc<crate::PredicateDeclaration>>)
{
	use crate::Formula;

	match formula
	{
		Formula::And(ref arguments)
		| Formula::IfAndOnlyIf(ref arguments)
		| Formula::Or(ref arguments) =>
			for argument in arguments
			{
				collect_predicate_declarations_in_formula(argument, &mut predicate_declarations);
			},
		Formula::Boolean(_)
		| Formula::Compare(_) => (),
		Formula::Exists(ref quantified_formula)
		| Formula::ForAll(ref quantified_formula) =>
			collect_predicate_declarations_in_formula(&quantified_formula.argument,
				&mut predicate_declarations),
		Formula::Implies(ref implies) =>
		{
			collect_predicate_declarations_in_formula(&implies.antecedent,
				&mut predicate_declarations);
			collect_predicate_declarations_in_formula(&implies.implication,
				&mut predicate_declarations);
		},
		Formula::Not(ref argument) =>
			collect_predicate_declarations_in_formula(argument, &mut predicate_declarations),
		Formula::Predicate(ref predicate) =>
			if !predicate_declarations.contains(&predicate.declaration)
			{
				predicate_declarations.insert(std::rc::Rc::clone(&predicate.declaration));
			},
	}
}

pub(crate) fn collect_predicate_declarations(completed_definition: &crate::Formula)
	-> std::collections::BTreeSet<std::rc::Rc<crate::PredicateDeclaration>>
{
	let mut predicate_declarations = std::collections::BTreeSet::new();

	use crate::Formula;

	let false_ = crate::Formula::false_();

	// TODO: refactor
	let (_completed_definition_predicate, completed_definition) = match completed_definition
	{
		Formula::ForAll(quantified_expression) => match *quantified_expression.argument
		{
			Formula::IfAndOnlyIf(ref arguments) =>
			{
				assert_eq!(arguments.len(), 2, "invalid completed definition");

				match arguments[0]
				{
					Formula::Predicate(ref predicate) => (predicate, &arguments[1]),
					_ => unreachable!("invalid completed definition"),
				}
			},
			Formula::Not(ref argument) => match **argument
			{
				Formula::Predicate(ref predicate) => (predicate, &false_),
				_ => unreachable!("invalid completed definition"),
			},
			_ => unreachable!("invalid completed definition"),
		},
		Formula::IfAndOnlyIf(ref arguments) =>
		{
			assert_eq!(arguments.len(), 2, "invalid completed definition");

			match arguments[0]
			{
				Formula::Predicate(ref predicate) => (predicate, &arguments[1]),
				_ => unreachable!("invalid completed definition"),
			}
		},
		Formula::Not(ref argument) => match **argument
		{
			Formula::Predicate(ref predicate) => (predicate, &false_),
			_ => unreachable!("invalid completed definition"),
		},
		_ => unreachable!("invalid completed definition"),
	};

	collect_predicate_declarations_in_formula(completed_definition, &mut predicate_declarations);

	predicate_declarations
}
