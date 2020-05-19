#[derive(Clone, Copy, Eq, PartialEq)]
enum SimplificationResult
{
	Simplified,
	NotSimplified,
}

impl SimplificationResult
{
	fn or(&self, other: SimplificationResult) -> SimplificationResult
	{
		match (self, other)
		{
			(SimplificationResult::NotSimplified, SimplificationResult::NotSimplified)
				=> SimplificationResult::NotSimplified,
			_ => SimplificationResult::Simplified,
		}
	}
}

fn remove_unnecessary_exists_parameters(formula: &mut foliage::Formula) -> SimplificationResult
{
	use foliage::Formula;

	match formula
	{
		Formula::And(ref mut arguments)
		| Formula::IfAndOnlyIf(ref mut arguments)
		| Formula::Or(ref mut arguments) =>
		{
			let mut simplification_result = SimplificationResult::NotSimplified;

			for argument in arguments
			{
				simplification_result = simplification_result.or(
					remove_unnecessary_exists_parameters(argument));
			}

			simplification_result
		},
		Formula::Boolean(_)
		| Formula::Compare(_)
		| Formula::Predicate(_) => SimplificationResult::NotSimplified,
		Formula::Exists(ref mut quantified_formula) =>
		{
			let mut simplification_result =
				remove_unnecessary_exists_parameters(&mut quantified_formula.argument);

			let arguments = match *quantified_formula.argument
			{
				Formula::And(ref mut arguments) => arguments,
				_ => return remove_unnecessary_exists_parameters(&mut quantified_formula.argument),
			};

			// TODO: do not copy parameters, use std::vec::Vec::retain instead
			quantified_formula.parameters =
				std::rc::Rc::new(quantified_formula.parameters.iter().filter_map(
					|parameter|
					{
						let assignment = arguments.iter().enumerate().find_map(
							|(argument_index, argument)|
							{
								let (left, right) = match argument
								{
									Formula::Compare(foliage::Compare{
										operator: foliage::ComparisonOperator::Equal, ref left,
										ref right})
										=> (left, right),
									_ => return None,
								};

								if let foliage::Term::Variable(ref variable) = **left
								{
									if variable.declaration == *parameter
										&& !crate::term_contains_variable(right, parameter)
									{
										// TODO: avoid copy
										return Some((argument_index, crate::copy_term(right)));
									}
								}

								if let foliage::Term::Variable(ref variable) = **right
								{
									if variable.declaration == *parameter
										&& !crate::term_contains_variable(left, parameter)
									{
										// TODO: avoid copy
										return Some((argument_index, crate::copy_term(left)));
									}
								}

								None
							});

						if let Some((assignment_index, assigned_term)) = assignment
						{
							arguments.remove(assignment_index);

							for argument in arguments.iter_mut()
							{
								crate::replace_variable_in_formula_with_term(argument, parameter,
									&assigned_term);
							}

							simplification_result = SimplificationResult::Simplified;

							return None;
						}

						Some(std::rc::Rc::clone(parameter))
					})
					.collect());

			simplification_result
		}
		Formula::ForAll(ref mut quantified_formula) =>
			remove_unnecessary_exists_parameters(&mut quantified_formula.argument),
		Formula::Implies(ref mut implies) =>
			remove_unnecessary_exists_parameters(&mut implies.antecedent)
				.or(remove_unnecessary_exists_parameters(&mut implies.implication)),
		Formula::Not(ref mut argument) =>
			remove_unnecessary_exists_parameters(argument),
	}
}

fn simplify_quantified_formulas_without_parameters(formula: &mut foliage::Formula)
	-> SimplificationResult
{
	use foliage::Formula;

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
		{
			let mut simplification_result = SimplificationResult::NotSimplified;

			for mut argument in arguments
			{
				simplification_result = simplification_result.or(
					simplify_quantified_formulas_without_parameters(&mut argument));
			}

			simplification_result
		},
		Formula::Boolean(_)
		| Formula::Compare(_)
		| Formula::Predicate(_) => SimplificationResult::NotSimplified,
		Formula::Exists(quantified_formula)
		| Formula::ForAll(quantified_formula) =>
		{
			if quantified_formula.parameters.is_empty()
			{
				// TODO: remove workaround
				let mut argument = foliage::Formula::false_();
				std::mem::swap(&mut argument, &mut quantified_formula.argument);

				*formula = argument;

				return SimplificationResult::Simplified;
			}

			simplify_quantified_formulas_without_parameters(&mut quantified_formula.argument)
		},
		Formula::Implies(ref mut implies) =>
			simplify_quantified_formulas_without_parameters(&mut implies.antecedent)
				.or(simplify_quantified_formulas_without_parameters(&mut implies.implication)),
		Formula::Not(ref mut argument) =>
			simplify_quantified_formulas_without_parameters(argument),
	}
}

fn simplify_trivial_n_ary_formulas(formula: &mut foliage::Formula) -> SimplificationResult
{
	use foliage::Formula;

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments) if arguments.is_empty() =>
		{
			*formula = foliage::Formula::true_();

			return SimplificationResult::Simplified;
		},
		| Formula::Or(arguments) if arguments.is_empty() =>
		{
			*formula = foliage::Formula::false_();

			return SimplificationResult::Simplified;
		},
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
		{
			if arguments.len() == 1
			{
				*formula = arguments.remove(0);

				return SimplificationResult::Simplified;
			}

			let mut simplification_result = SimplificationResult::NotSimplified;

			for mut argument in arguments
			{
				simplification_result = simplification_result.or(
					simplify_trivial_n_ary_formulas(&mut argument));
			}

			simplification_result
		},
		Formula::Boolean(_)
		| Formula::Compare(_)
		| Formula::Predicate(_) => SimplificationResult::NotSimplified,
		Formula::Exists(ref mut quantified_formula)
		| Formula::ForAll(ref mut quantified_formula) =>
			simplify_trivial_n_ary_formulas(&mut quantified_formula.argument),
		Formula::Implies(ref mut implies) =>
			simplify_trivial_n_ary_formulas(&mut implies.antecedent)
				.or(simplify_trivial_n_ary_formulas(&mut implies.implication)),
		Formula::Not(ref mut argument) =>
			simplify_trivial_n_ary_formulas(argument),
	}
}

pub(crate) fn simplify(formula: &mut foliage::Formula)
{
	loop
	{
		if remove_unnecessary_exists_parameters(formula) == SimplificationResult::Simplified
			|| simplify_quantified_formulas_without_parameters(formula)
				== SimplificationResult::Simplified
			|| simplify_trivial_n_ary_formulas(formula) == SimplificationResult::Simplified
		{
			log::debug!("cool, simplified!");

			continue;
		}

		log::debug!("nope, that’s it");

		break;
	}
}
