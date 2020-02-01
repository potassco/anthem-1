pub(crate) fn translate_body_term<F1, F2, F3>(body_term: &clingo::ast::Term, sign: clingo::ast::Sign,
	mut find_or_create_function_declaration: F1, mut find_or_create_predicate_declaration: F2,
	mut find_or_create_variable_declaration: F3)
	-> Result<foliage::Formula, crate::Error>
where
	F1: crate::translate::common::FindOrCreateFunctionDeclaration,
	F2: crate::translate::common::FindOrCreatePredicateDeclaration,
	F3: crate::translate::common::FindOrCreateVariableDeclaration,
{
	let function = match body_term.term_type()
	{
		clingo::ast::TermType::Function(value) => value,
		_ => return Err(crate::Error::new_unsupported_language_feature("only functions supported as body terms")),
	};

	let function_name = function.name().map_err(|error| crate::Error::new_decode_identifier(error))?;

	let predicate_declaration = find_or_create_predicate_declaration(function_name,
		function.arguments().len());

	let parameters = function.arguments().iter().map(|_| std::rc::Rc::new(
		foliage::VariableDeclaration
		{
			name: "<anonymous>".to_string(),
		}))
		.collect::<Vec<_>>();

	let predicate_arguments = parameters.iter().map(
		|parameter| Box::new(foliage::Term::Variable(foliage::Variable{declaration: std::rc::Rc::clone(parameter)})))
		.collect::<Vec<_>>();

	let predicate = foliage::Predicate
	{
		declaration: predicate_declaration,
		arguments: predicate_arguments,
	};

	let predicate_literal = match sign
	{
		clingo::ast::Sign::None
		| clingo::ast::Sign::DoubleNegation
			=> foliage::Formula::Predicate(predicate),
		clingo::ast::Sign::Negation
			=> foliage::Formula::Not(Box::new(foliage::Formula::Predicate(predicate))),
	};

	if function.arguments().is_empty()
	{
		return Ok(predicate_literal);
	}

	let mut i = 0;

	let mut arguments = function.arguments().iter().map(|x|
		{
			let result = crate::translate::common::choose_value_in_term(x, &parameters[i],
				&mut find_or_create_function_declaration, &mut find_or_create_variable_declaration)
				.map(|x| Box::new(x));
			i += 1;
			result
		})
		.collect::<Result<Vec<_>, _>>()?;

	arguments.push(Box::new(predicate_literal));

	let and = foliage::Formula::And(arguments);

	Ok(foliage::Formula::Exists(foliage::Exists
	{
		parameters,
		argument: Box::new(and),
	}))
}

pub(crate) fn translate_body_literal<F1, F2, F3>(body_literal: &clingo::ast::BodyLiteral,
	mut find_or_create_function_declaration: F1, mut find_or_create_predicate_declaration: F2,
	mut find_or_create_variable_declaration: F3)
	-> Result<foliage::Formula, crate::Error>
where
	F1: crate::translate::common::FindOrCreateFunctionDeclaration,
	F2: crate::translate::common::FindOrCreatePredicateDeclaration,
	F3: crate::translate::common::FindOrCreateVariableDeclaration,
{
	match body_literal.sign()
	{
		clingo::ast::Sign::None => (),
		_ => return Err(crate::Error::new_unsupported_language_feature(
			"signed body literals")),
	}

	let literal = match body_literal.body_literal_type()
	{
		clingo::ast::BodyLiteralType::Literal(literal) => literal,
		_ => return Err(crate::Error::new_unsupported_language_feature(
			"only plain body literals supported")),
	};

	match literal.literal_type()
	{
		clingo::ast::LiteralType::Boolean(value) =>
		{
			match literal.sign()
			{
				clingo::ast::Sign::None => (),
				_ => return Err(crate::Error::new_logic("unexpected negated Boolean value")),
			}

			Ok(foliage::Formula::Boolean(value))
		},
		clingo::ast::LiteralType::Symbolic(term) => translate_body_term(term, literal.sign(),
			&mut find_or_create_function_declaration, &mut find_or_create_predicate_declaration,
			&mut find_or_create_variable_declaration),
		clingo::ast::LiteralType::Comparison(comparison) =>
		{
			let new_variable_declaration = || std::rc::Rc::new(foliage::VariableDeclaration
			{
				name: "<anonymous>".to_string()
			});

			let parameters = vec![new_variable_declaration(), new_variable_declaration()];

			let parameter_z1 = &parameters[0];
			let parameter_z2 = &parameters[1];

			let choose_z1_in_t1 = crate::translate::common::choose_value_in_term(comparison.left(), &parameter_z1,
				&mut find_or_create_function_declaration, &mut find_or_create_variable_declaration)?;
			let choose_z2_in_t2 = crate::translate::common::choose_value_in_term(comparison.right(), &parameter_z2,
				&mut find_or_create_function_declaration, &mut find_or_create_variable_declaration)?;

			let variable_1 = foliage::Variable
			{
				declaration: std::rc::Rc::clone(&parameter_z1),
			};

			let variable_2 = foliage::Variable
			{
				declaration: std::rc::Rc::clone(&parameter_z2),
			};

			let comparison_operator
				= crate::translate::common::translate_comparison_operator(comparison.comparison_type());

			let compare_z1_and_z2 = foliage::Comparison
			{
				operator: comparison_operator,
				left: Box::new(foliage::Term::Variable(variable_1)),
				right: Box::new(foliage::Term::Variable(variable_2)),
			};

			let and = foliage::Formula::And(vec![Box::new(choose_z1_in_t1),
				Box::new(choose_z2_in_t2), Box::new(foliage::Formula::Comparison(compare_z1_and_z2))]);

			Ok(foliage::Formula::Exists(foliage::Exists
			{
				parameters,
				argument: Box::new(and),
			}))
		},
		_ => Err(crate::Error::new_unsupported_language_feature(
			"body literals other than Booleans, terms, or comparisons")),
	}
}

pub(crate) fn translate_body<F1, F2, F3>(body_literals: &[clingo::ast::BodyLiteral],
	mut find_or_create_function_declaration: F1, mut find_or_create_predicate_declaration: F2,
	mut find_or_create_variable_declaration: F3)
	-> Result<foliage::Formulas, crate::Error>
where
	F1: crate::translate::common::FindOrCreateFunctionDeclaration,
	F2: crate::translate::common::FindOrCreatePredicateDeclaration,
	F3: crate::translate::common::FindOrCreateVariableDeclaration,
{
	body_literals.iter()
		.map(|body_literal| translate_body_literal(body_literal,
			&mut find_or_create_function_declaration, &mut find_or_create_predicate_declaration,
			&mut find_or_create_variable_declaration)
			.map(|value| Box::new(value)))
		.collect::<Result<foliage::Formulas, crate::Error>>()
}
