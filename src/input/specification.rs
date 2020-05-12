// TODO: refactor
fn term_assign_variable_declaration_domains<D>(term: &foliage::Term, declarations: &D)
	-> Result<(), crate::Error>
where
	D: crate::traits::AssignVariableDeclarationDomain,
{
	match term
	{
		foliage::Term::BinaryOperation(binary_operation) =>
		{
			term_assign_variable_declaration_domains(&binary_operation.left, declarations)?;
			term_assign_variable_declaration_domains(&binary_operation.right, declarations)?;
		},
		foliage::Term::Function(function) =>
			for argument in &function.arguments
			{
				term_assign_variable_declaration_domains(&argument, declarations)?;
			},
		foliage::Term::UnaryOperation(unary_operation) =>
			term_assign_variable_declaration_domains(&unary_operation.argument, declarations)?,
		foliage::Term::Variable(variable) =>
		{
			let domain =  match variable.declaration.name.chars().next()
			{
				Some('X')
				| Some('Y')
				| Some('Z') => crate::Domain::Program,
				Some('I')
				| Some('J')
				| Some('K')
				| Some('L')
				| Some('M')
				| Some('N') => crate::Domain::Integer,
				Some(_) => return Err(
					crate::Error::new_variable_name_not_allowed(variable.declaration.name.clone())),
				None => unreachable!(),
			};

			declarations.assign_variable_declaration_domain(&variable.declaration, domain);
		},
		_ => (),
	}

	Ok(())
}

fn formula_assign_variable_declaration_domains<D>(formula: &foliage::Formula, declarations: &D)
	-> Result<(), crate::Error>
where
	D: crate::traits::AssignVariableDeclarationDomain,
{
	match formula
	{
		foliage::Formula::And(arguments)
		| foliage::Formula::Or(arguments)
		| foliage::Formula::IfAndOnlyIf(arguments) =>
			for argument in arguments
			{
				formula_assign_variable_declaration_domains(&argument, declarations)?;
			},
		foliage::Formula::Compare(compare) =>
		{
			term_assign_variable_declaration_domains(&compare.left, declarations)?;
			term_assign_variable_declaration_domains(&compare.right, declarations)?;
		},
		foliage::Formula::Exists(quantified_formula)
		| foliage::Formula::ForAll(quantified_formula) =>
			formula_assign_variable_declaration_domains(&quantified_formula.argument,
				declarations)?,
		foliage::Formula::Implies(implies) =>
		{
			formula_assign_variable_declaration_domains(&implies.antecedent, declarations)?;
			formula_assign_variable_declaration_domains(&implies.implication, declarations)?;
		}
		foliage::Formula::Not(argument) =>
			formula_assign_variable_declaration_domains(&argument, declarations)?,
		foliage::Formula::Predicate(predicate) =>
			for argument in &predicate.arguments
			{
				term_assign_variable_declaration_domains(&argument, declarations)?;
			},
		_ => (),
	}

	Ok(())
}

fn open_formula<'i, D>(input: &'i str, declarations: &D)
	-> Result<(foliage::OpenFormula, &'i str), crate::Error>
where
	D: foliage::FindOrCreateFunctionDeclaration
		+ foliage::FindOrCreatePredicateDeclaration
		+ crate::traits::AssignVariableDeclarationDomain,
{
	let terminator_position = match input.find('.')
	{
		None => return Err(crate::Error::new_missing_statement_terminator()),
		Some(terminator_position) => terminator_position,
	};

	let (formula_input, remaining_input) = input.split_at(terminator_position);
	let mut remaining_input_characters = remaining_input.chars();
	remaining_input_characters.next();
	let remaining_input = remaining_input_characters.as_str();

	let open_formula = foliage::parse::formula(formula_input, declarations)
		.map_err(|error| crate::Error::new_parse_formula(error))?;

	formula_assign_variable_declaration_domains(&open_formula.formula, declarations)?;

	let open_formula = foliage::OpenFormula
	{
		free_variable_declarations: open_formula.free_variable_declarations,
		formula: open_formula.formula,
	};

	Ok((open_formula, remaining_input))
}

fn formula<'i, D>(mut input: &'i str, declarations: &D)
	-> Result<(foliage::Formula, &'i str), crate::Error>
where
	D: foliage::FindOrCreateFunctionDeclaration
		+ foliage::FindOrCreatePredicateDeclaration
		+ crate::traits::AssignVariableDeclarationDomain,
{
	let (open_formula, remaining_input) = open_formula(input, declarations)?;

	input = remaining_input;

	if !open_formula.free_variable_declarations.is_empty()
	{
		return Err(crate::Error::new_formula_not_closed(open_formula.free_variable_declarations));
	}

	Ok((open_formula.formula, input))
}

fn formula_statement_body<'i>(mut input: &'i str, problem: &crate::Problem)
	-> Result<(foliage::Formula, &'i str), crate::Error>
{
	input = input.trim_start();

	let mut input_characters = input.chars();

	input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	formula(input, problem)
}

fn domain_specifier<'i>(mut input: &'i str)
	-> Result<(Option<crate::Domain>, &'i str), crate::Error>
{
	let original_input = input;
	input = input.trim_start();

	if input.starts_with("->")
	{
		let mut input_characters = input.chars();
		input_characters.next();
		input_characters.next();

		input = input_characters.as_str().trim_start();

		let (identifier, remaining_input) =
			foliage::parse::tokens::identifier(input)
				.ok_or_else(|| crate::Error::new_expected_identifier())?;

		input = remaining_input;

		match identifier
		{
			"integer" => Ok((Some(crate::Domain::Integer), input)),
			"program" => Ok((Some(crate::Domain::Program), input)),
			_ => return Err(crate::Error::new_unknown_domain_identifier(identifier.to_string())),
		}
	}
	else
	{
		Ok((None, original_input))
	}
}

fn predicate_arity_specifier<'i>(mut input: &'i str)
	-> Result<(Option<usize>, &'i str), crate::Error>
{
	let original_input = input;
	input = input.trim_start();

	let mut input_characters = input.chars();

	if input_characters.next() == Some('/')
	{
		input = input_characters.as_str().trim_start();

		let (arity, remaining_input) = foliage::parse::tokens::number(input)
			.map_err(|error| crate::Error::new_parse_predicate_declaration().with(error))?
			.ok_or_else(|| crate::Error::new_parse_predicate_declaration())?;

		input = remaining_input;

		Ok((Some(arity), input))
	}
	else
	{
		Ok((None, original_input))
	}
}

fn expect_statement_terminator<'i>(mut input: &'i str) -> Result<&'i str, crate::Error>
{
	input = input.trim_start();

	let mut input_characters = input.chars();

	if input_characters.next() != Some('.')
	{
		return Err(crate::Error::new_missing_statement_terminator())
	}

	input = input_characters.as_str();

	Ok(input)
}

fn input_statement_body<'i>(mut input: &'i str, problem: &crate::Problem)
	-> Result<&'i str, crate::Error>
{
	input = input.trim_start();

	let mut input_characters = input.chars();

	let remaining_input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	input = remaining_input;

	loop
	{
		input = input.trim_start();

		let (constant_or_predicate_name, remaining_input) =
			foliage::parse::tokens::identifier(input)
				.ok_or_else(|| crate::Error::new_expected_identifier())?;

		input = remaining_input.trim_start();

		// Parse input predicate specifiers
		if let (Some(arity), remaining_input) = predicate_arity_specifier(input)?
		{
			input = remaining_input;

			let mut input_predicate_declarations =
				problem.input_predicate_declarations.borrow_mut();

			use foliage::FindOrCreatePredicateDeclaration as _;

			let predicate_declaration =
				problem.find_or_create_predicate_declaration(constant_or_predicate_name, arity);

			input_predicate_declarations.insert(predicate_declaration);
		}
		// Parse input constant specifiers
		else
		{
			let (domain, remaining_input) = match domain_specifier(input)?
			{
				(Some(domain), remaining_input) => (domain, remaining_input),
				(None, remaining_input) => (crate::Domain::Program, remaining_input),
			};

			input = remaining_input;

			let mut input_constant_declarations =
				problem.input_constant_declarations.borrow_mut();

			use foliage::FindOrCreateFunctionDeclaration as _;

			let constant_declaration =
				problem.find_or_create_function_declaration(constant_or_predicate_name, 0);

			input_constant_declarations.insert(std::rc::Rc::clone(&constant_declaration));

			let mut input_constant_declaration_domains =
				problem.input_constant_declaration_domains.borrow_mut();

			input_constant_declaration_domains.insert(constant_declaration, domain);
		}

		let mut input_characters = input.chars();

		match input_characters.next()
		{
			Some(',') => input = input_characters.as_str(),
			_ => break,
		}
	}

	expect_statement_terminator(input)
}

fn output_statement_body<'i>(mut input: &'i str, problem: &crate::Problem)
	-> Result<&'i str, crate::Error>
{
	input = input.trim_start();

	let mut input_characters = input.chars();

	let remaining_input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	input = remaining_input;

	loop
	{
		input = input.trim_start();

		let (constant_or_predicate_name, remaining_input) =
			foliage::parse::tokens::identifier(input)
				.ok_or_else(|| crate::Error::new_expected_identifier())?;

		input = remaining_input.trim_start();

		// Only accept output predicate specifiers
		if let (Some(arity), remaining_input) = predicate_arity_specifier(input)?
		{
			input = remaining_input;

			let mut output_predicate_declarations =
				problem.output_predicate_declarations.borrow_mut();

			use foliage::FindOrCreatePredicateDeclaration as _;

			let predicate_declaration =
				problem.find_or_create_predicate_declaration(constant_or_predicate_name, arity);

			output_predicate_declarations.insert(predicate_declaration);
		}
		else
		{
			return Err(crate::Error::new_expected_predicate_specifier());
		}

		let mut input_characters = input.chars();

		match input_characters.next()
		{
			Some(',') => input = input_characters.as_str(),
			_ => break,
		}
	}

	expect_statement_terminator(input)
}

pub(crate) fn parse_specification(mut input: &str, problem: &crate::Problem)
	-> Result<(), crate::Error>
{
	loop
	{
		input = input.trim_start();

		if input.is_empty()
		{
			return Ok(());
		}

		let (identifier, remaining_input) = match foliage::parse::tokens::identifier(input)
		{
			Some(identifier) => identifier,
			None => return Err(crate::Error::new_expected_statement()),
		};

		input = remaining_input;

		match identifier
		{
			"axiom" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;
				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Axiom, formula);

				problem.add_statement(crate::problem::SectionKind::Axioms, statement);

				continue;
			},
			"assume" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;
				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Assumption, formula);

				problem.add_statement(crate::problem::SectionKind::Assumptions, statement);

				continue;
			},
			"lemma" =>
			{
				input = input.trim_start();

				let mut input_characters = input.chars();

				let (proof_direction, remaining_input) = match input_characters.next()
				{
					Some('(') =>
					{
						// TODO: refactor
						input = input_characters.as_str().trim_start();

						let (proof_direction, remaining_input) = match
							foliage::parse::tokens::identifier(input)
						{
							Some(("forward", remaining_input)) =>
								(crate::problem::ProofDirection::Forward, remaining_input),
							Some(("backward", remaining_input)) =>
								(crate::problem::ProofDirection::Backward, remaining_input),
							Some(("both", remaining_input)) =>
								(crate::problem::ProofDirection::Both, remaining_input),
							Some((identifier, _)) =>
								return Err(crate::Error::new_unknown_proof_direction(
									identifier.to_string())),
							None => (crate::problem::ProofDirection::Both, input),
						};

						input = remaining_input.trim_start();

						let mut input_characters = input.chars();

						if input_characters.next() != Some(')')
						{
							return Err(crate::Error::new_unmatched_parenthesis());
						}

						input = input_characters.as_str();

						(proof_direction, input)
					},
					Some(_)
					| None => (crate::problem::ProofDirection::Both, remaining_input),
				};

				input = remaining_input;

				let (formula, remaining_input) = formula_statement_body(input, problem)?;

				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Lemma(proof_direction), formula);

				problem.add_statement(crate::problem::SectionKind::Lemmas, statement);

				continue;
			},
			"assert" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;

				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Assertion, formula);

				problem.add_statement(crate::problem::SectionKind::Assertions, statement);

				continue;
			},
			"input" => input = input_statement_body(input, problem)?,
			"output" => input = output_statement_body(input, problem)?,
			identifier => return Err(crate::Error::new_unknown_statement(identifier.to_string())),
		}
	}
}
