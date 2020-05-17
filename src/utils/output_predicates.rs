fn formula_contains_predicate(formula: &foliage::Formula,
	predicate_declaration: &foliage::PredicateDeclaration)
	-> bool
{
	use foliage::Formula;

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

fn replace_predicate_in_formula<D>(formula: &mut foliage::Formula,
	predicate_to_replace: &foliage::Predicate, replacement_formula: &foliage::Formula,
	declarations: &D)
where
	D: crate::traits::VariableDeclarationDomain + crate::traits::AssignVariableDeclarationDomain,
{
	use foliage::{Formula, Term};

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
			for mut argument in arguments
			{
				replace_predicate_in_formula(&mut argument, predicate_to_replace,
					replacement_formula, declarations);
			},
		Formula::Boolean(_)
		| Formula::Compare(_) => (),
		Formula::Exists(quantified_expression)
		| Formula::ForAll(quantified_expression) =>
			replace_predicate_in_formula(&mut quantified_expression.argument, predicate_to_replace,
				replacement_formula, declarations),
		Formula::Implies(implies) =>
		{
			replace_predicate_in_formula(&mut implies.antecedent, predicate_to_replace,
				replacement_formula, declarations);
			replace_predicate_in_formula(&mut implies.implication, predicate_to_replace,
				replacement_formula, declarations);
		},
		Formula::Not(argument) =>
			replace_predicate_in_formula(argument, predicate_to_replace, replacement_formula,
				declarations),
		Formula::Predicate(predicate) =>
			if predicate.declaration == predicate_to_replace.declaration
			{
				let mut replacement_formula =
					crate::utils::copy_formula(replacement_formula, declarations);

				for (index, argument) in predicate.arguments.iter().enumerate()
				{
					let variable_declaration = match &predicate_to_replace.arguments[index]
					{
						Term::Variable(variable) => &variable.declaration,
						_ => panic!("invalid completed definition"),
					};

					crate::utils::replace_variable_in_formula_with_term(&mut replacement_formula,
						&variable_declaration, argument);
				}

				*formula = replacement_formula;
			},
	}
}

pub(crate) fn replace_predicate_in_formula_with_completed_definition<D>(
	formula: &mut foliage::Formula, completed_definition: &foliage::Formula, declarations: &D)
	-> Result<(), crate::Error>
where
	D: crate::traits::VariableDeclarationDomain + crate::traits::AssignVariableDeclarationDomain,
{
	let false_ = foliage::Formula::false_();

	// TODO: refactor
	let (completed_definition_predicate, completed_definition) = match completed_definition
	{
		foliage::Formula::ForAll(quantified_expression) => match *quantified_expression.argument
		{
			foliage::Formula::IfAndOnlyIf(ref arguments) =>
			{
				assert_eq!(arguments.len(), 2, "invalid completed definition");

				match arguments[0]
				{
					foliage::Formula::Predicate(ref predicate) => (predicate, &arguments[1]),
					_ => panic!("invalid completed definition"),
				}
			},
			foliage::Formula::Not(ref argument) => match **argument
			{
				foliage::Formula::Predicate(ref predicate) => (predicate, &false_),
				_ => panic!("invalid completed definition"),
			},
			_ => panic!("invalid completed definition"),
		},
		foliage::Formula::IfAndOnlyIf(ref arguments) =>
		{
			assert_eq!(arguments.len(), 2, "invalid completed definition");

			match arguments[0]
			{
				foliage::Formula::Predicate(ref predicate) => (predicate, &arguments[1]),
				_ => panic!("invalid completed definition"),
			}
		},
		foliage::Formula::Not(ref argument) => match **argument
		{
			foliage::Formula::Predicate(ref predicate) => (predicate, &false_),
			_ => panic!("invalid completed definition"),
		},
		_ => panic!("invalid completed definition"),
	};

	// Predicates can only be substituted by their completed definitions if there is no cycle.
	// For example, if the completed definition of p/1 references q/1 and vice versa, neither can
	// be replaced with the completed definition of the other
	if formula_contains_predicate(completed_definition, &completed_definition_predicate.declaration)
		&& formula_contains_predicate(formula, &completed_definition_predicate.declaration)
	{
		return Err(crate::Error::new_cannot_hide_predicate(
			std::rc::Rc::clone(&completed_definition_predicate.declaration)));
	}

	replace_predicate_in_formula(formula, completed_definition_predicate, completed_definition,
		declarations);

	Ok(())
}
