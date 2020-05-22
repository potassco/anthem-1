struct IDs
{
	program_variable_id: usize,
	integer_variable_id: usize,
}

impl IDs
{
	pub fn new() -> Self
	{
		Self
		{
			program_variable_id: 0,
			integer_variable_id: 0,
		}
	}
}

fn reset_variable_name(variable_declaration: &crate::VariableDeclaration)
{
	let domain = match variable_declaration.domain()
	{
		Ok(domain) => domain,
		Err(_) => unreachable!("all variable domains should be assigned at this point"),
	};

	let ref mut variable_name = *variable_declaration.name.borrow_mut();

	match variable_name
	{
		crate::VariableName::Generated(ref mut generated_variable_name) =>
			generated_variable_name.id = None,
		crate::VariableName::UserDefined(_) =>
			*variable_name = crate::VariableName::Generated(
				crate::GeneratedVariableName
				{
					domain,
					id: None,
				}),
	}
}

fn reset_variable_names_in_term(term: &mut crate::Term)
{
	use foliage::Term;

	match term
	{
		Term::BinaryOperation(ref mut binary_operation) =>
		{
			reset_variable_names_in_term(&mut *binary_operation.left);
			reset_variable_names_in_term(&mut *binary_operation.right);
		},
		Term::Boolean(_)
		| Term::Integer(_)
		| Term::SpecialInteger(_)
		| Term::String(_) => (),
		Term::Function(ref mut function) =>
			for mut argument in &mut function.arguments
			{
				reset_variable_names_in_term(&mut argument);
			},
		Term::UnaryOperation(ref mut unary_operation) =>
			reset_variable_names_in_term(&mut *unary_operation.argument),
		Term::Variable(ref mut variable) => reset_variable_name(&variable.declaration),
	}
}

fn reset_variable_names_in_formula(formula: &mut crate::Formula)
{
	use foliage::Formula;

	match formula
	{
		Formula::And(ref mut arguments)
		| Formula::IfAndOnlyIf(ref mut arguments)
		| Formula::Or(ref mut arguments) =>
			for argument in arguments
			{
				reset_variable_names_in_formula(argument);
			},
		Formula::Boolean(_) => (),
		Formula::Compare(ref mut compare) =>
		{
			reset_variable_names_in_term(&mut *compare.left);
			reset_variable_names_in_term(&mut *compare.right);
		},
		Formula::Exists(ref mut quantified_formula)
		| Formula::ForAll(ref mut quantified_formula) =>
		{
			for ref parameter in &*quantified_formula.parameters
			{
				reset_variable_name(&parameter);
			}

			reset_variable_names_in_formula(&mut *quantified_formula.argument);
		},
		Formula::Implies(ref mut implies) =>
		{
			reset_variable_names_in_formula(&mut *implies.antecedent);
			reset_variable_names_in_formula(&mut *implies.implication);
		},
		Formula::Not(ref mut argument) => reset_variable_names_in_formula(argument),
		Formula::Predicate(ref mut predicate) =>
			for mut argument in &mut predicate.arguments
			{
				reset_variable_names_in_term(&mut argument);
			},
	}
}

fn set_variable_name(variable_declaration: &crate::VariableDeclaration, ids: &mut IDs)
{
	match *variable_declaration.name.borrow_mut()
	{
		crate::VariableName::Generated(ref mut generated_variable_name) =>
		{
			if generated_variable_name.id.is_some()
			{
				return;
			}

			match generated_variable_name.domain
			{
				crate::Domain::Program =>
				{
					generated_variable_name.id = Some(ids.program_variable_id);
					ids.program_variable_id += 1;
				},
				crate::Domain::Integer =>
				{
					generated_variable_name.id = Some(ids.integer_variable_id);
					ids.integer_variable_id += 1;
				},
			}
		},
		crate::VariableName::UserDefined(_) => (),
	}
}

fn set_variable_names_in_term(term: &mut crate::Term, ids: &mut IDs)
{
	use foliage::Term;

	match term
	{
		Term::BinaryOperation(ref mut binary_operation) =>
		{
			set_variable_names_in_term(&mut *binary_operation.left, ids);
			set_variable_names_in_term(&mut *binary_operation.right, ids);
		},
		Term::Boolean(_)
		| Term::Integer(_)
		| Term::SpecialInteger(_)
		| Term::String(_) => (),
		Term::Function(ref mut function) =>
			for mut argument in &mut function.arguments
			{
				set_variable_names_in_term(&mut argument, ids);
			},
		Term::UnaryOperation(ref mut unary_operation) =>
			set_variable_names_in_term(&mut *unary_operation.argument, ids),
		Term::Variable(ref mut variable) => set_variable_name(&variable.declaration, ids),
	}
}

fn set_variable_names_in_formula(formula: &mut crate::Formula, ids: &mut IDs)
{
	use foliage::Formula;

	match formula
	{
		Formula::And(ref mut arguments)
		| Formula::IfAndOnlyIf(ref mut arguments)
		| Formula::Or(ref mut arguments) =>
			for argument in arguments
			{
				set_variable_names_in_formula(argument, ids);
			},
		Formula::Boolean(_) => (),
		Formula::Compare(ref mut compare) =>
		{
			set_variable_names_in_term(&mut *compare.left, ids);
			set_variable_names_in_term(&mut *compare.right, ids);
		},
		Formula::Exists(ref mut quantified_formula)
		| Formula::ForAll(ref mut quantified_formula) =>
		{
			for ref parameter in &*quantified_formula.parameters
			{
				set_variable_name(&parameter, ids);
			}

			set_variable_names_in_formula(&mut *quantified_formula.argument, ids);
		},
		Formula::Implies(ref mut implies) =>
			match implies.direction
			{
				foliage::ImplicationDirection::LeftToRight =>
				{
					set_variable_names_in_formula(&mut *implies.antecedent, ids);
					set_variable_names_in_formula(&mut *implies.implication, ids);
				},
				foliage::ImplicationDirection::RightToLeft =>
				{
					set_variable_names_in_formula(&mut *implies.implication, ids);
					set_variable_names_in_formula(&mut *implies.antecedent, ids);
				},
		},
		Formula::Not(ref mut argument) => set_variable_names_in_formula(argument, ids),
		Formula::Predicate(ref mut predicate) =>
			for mut argument in &mut predicate.arguments
			{
				set_variable_names_in_term(&mut argument, ids);
			},
	}
}

pub(crate) fn autoname_variables(formula: &mut crate::Formula)
{
	reset_variable_names_in_formula(formula);
	set_variable_names_in_formula(formula, &mut IDs::new());
}
