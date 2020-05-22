fn open_formula<'i, D>(input: &'i str, declarations: &D)
	-> Result<(crate::OpenFormula, &'i str), crate::Error>
where
	D: foliage::FindOrCreateFunctionDeclaration<crate::FoliageFlavor>
		+ foliage::FindOrCreatePredicateDeclaration<crate::FoliageFlavor>
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

	let open_formula = crate::OpenFormula
	{
		free_variable_declarations: open_formula.free_variable_declarations,
		formula: open_formula.formula,
	};

	Ok((open_formula, remaining_input))
}

fn formula<'i, D>(mut input: &'i str, declarations: &D)
	-> Result<(crate::Formula, &'i str), crate::Error>
where
	D: foliage::FindOrCreateFunctionDeclaration<crate::FoliageFlavor>
		+ foliage::FindOrCreatePredicateDeclaration<crate::FoliageFlavor>
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
	-> Result<(crate::Formula, &'i str), crate::Error>
{
	input = foliage::parse::tokens::trim_start(input);

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
	input = foliage::parse::tokens::trim_start(input);

	if input.starts_with("->")
	{
		let mut input_characters = input.chars();
		input_characters.next();
		input_characters.next();

		input = foliage::parse::tokens::trim_start(input_characters.as_str());

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
	input = foliage::parse::tokens::trim_start(input);

	let mut input_characters = input.chars();

	if input_characters.next() == Some('/')
	{
		input = foliage::parse::tokens::trim_start(input_characters.as_str());

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
	input = foliage::parse::tokens::trim_start(input);

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
	input = foliage::parse::tokens::trim_start(input);

	let mut input_characters = input.chars();

	let remaining_input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	input = remaining_input;

	loop
	{
		input = foliage::parse::tokens::trim_start(input);

		let (constant_or_predicate_name, remaining_input) =
			foliage::parse::tokens::identifier(input)
				.ok_or_else(|| crate::Error::new_expected_identifier())?;

		input = foliage::parse::tokens::trim_start(remaining_input);

		// Parse input predicate specifiers
		if let (Some(arity), remaining_input) = predicate_arity_specifier(input)?
		{
			input = remaining_input;

			use foliage::FindOrCreatePredicateDeclaration as _;

			let predicate_declaration =
				problem.find_or_create_predicate_declaration(constant_or_predicate_name, arity);

			*predicate_declaration.is_input.borrow_mut() = true;
		}
		// Parse input constant specifiers
		else
		{
			// TODO: detect conflicting domain specifiers (for example: n -> program, n -> integer)
			let (domain, remaining_input) = match domain_specifier(input)?
			{
				(Some(domain), remaining_input) => (domain, remaining_input),
				(None, remaining_input) => (crate::Domain::Program, remaining_input),
			};

			input = remaining_input;

			use foliage::FindOrCreateFunctionDeclaration as _;

			let constant_declaration =
				problem.find_or_create_function_declaration(constant_or_predicate_name, 0);

			*constant_declaration.is_input.borrow_mut() = true;
			*constant_declaration.domain.borrow_mut() = domain;
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
	input = foliage::parse::tokens::trim_start(input);

	let mut input_characters = input.chars();

	let remaining_input = match input_characters.next()
	{
		Some(':') => input_characters.as_str(),
		_ => return Err(crate::Error::new_expected_colon()),
	};

	input = remaining_input;

	loop
	{
		input = foliage::parse::tokens::trim_start(input);

		let (constant_or_predicate_name, remaining_input) =
			foliage::parse::tokens::identifier(input)
				.ok_or_else(|| crate::Error::new_expected_identifier())?;

		input = foliage::parse::tokens::trim_start(remaining_input);

		// Only accept output predicate specifiers
		if let (Some(arity), remaining_input) = predicate_arity_specifier(input)?
		{
			input = remaining_input;

			use foliage::FindOrCreatePredicateDeclaration as _;

			let predicate_declaration =
				problem.find_or_create_predicate_declaration(constant_or_predicate_name, arity);

			*predicate_declaration.is_output.borrow_mut() = true;
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
		input = foliage::parse::tokens::trim_start(input);

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
				input = foliage::parse::tokens::trim_start(input);

				let mut input_characters = input.chars();

				let (proof_direction, remaining_input) = match input_characters.next()
				{
					Some('(') =>
					{
						// TODO: refactor
						input = foliage::parse::tokens::trim_start(input_characters.as_str());

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

						input = foliage::parse::tokens::trim_start(remaining_input);

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
			"spec" =>
			{
				let (formula, remaining_input) = formula_statement_body(input, problem)?;

				input = remaining_input;

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::Spec, formula);

				problem.add_statement(crate::problem::SectionKind::Specs, statement);

				continue;
			},
			"input" => input = input_statement_body(input, problem)?,
			"output" => input = output_statement_body(input, problem)?,
			identifier => return Err(crate::Error::new_unknown_statement(identifier.to_string())),
		}
	}
}
