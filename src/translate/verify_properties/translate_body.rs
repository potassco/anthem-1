pub(crate) fn translate_body_term<C>(body_term: &clingo::ast::Term, sign: clingo::ast::Sign,
	context: &C)
	-> Result<foliage::Formula, crate::Error>
where
	C: crate::translate::common::GetOrCreateVariableDeclaration
		+ crate::translate::common::GetOrCreateFunctionDeclaration
		+ crate::translate::common::GetOrCreatePredicateDeclaration
		+ crate::translate::common::AssignVariableDeclarationDomain
{
	let function = match body_term.term_type()
	{
		clingo::ast::TermType::Function(value) => value,
		_ => return Err(crate::Error::new_unsupported_language_feature("only functions supported as body terms")),
	};

	let function_name = function.name().map_err(|error| crate::Error::new_decode_identifier(error))?;

	let predicate_declaration = context.get_or_create_predicate_declaration(function_name,
		function.arguments().len());

	let parameters = function.arguments().iter().map(|_|
		{
			let variable_declaration = std::rc::Rc::new(
				foliage::VariableDeclaration::new("<anonymous>".to_string()));
			context.assign_variable_declaration_domain(&variable_declaration,
				crate::translate::common::Domain::Program);
			variable_declaration
		})
		.collect::<foliage::VariableDeclarations>();
	let parameters = std::rc::Rc::new(parameters);

	let predicate_arguments = parameters.iter().map(
		|parameter| Box::new(foliage::Term::variable(parameter)))
		.collect::<Vec<_>>();

	let predicate = foliage::Formula::predicate(&predicate_declaration, predicate_arguments);

	let predicate_literal = match sign
	{
		clingo::ast::Sign::None
		| clingo::ast::Sign::DoubleNegation
			=> predicate,
		clingo::ast::Sign::Negation
			=> foliage::Formula::not(Box::new(predicate)),
	};

	if function.arguments().is_empty()
	{
		return Ok(predicate_literal);
	}

	assert_eq!(function.arguments().len(), parameters.len());

	let mut parameters_iterator = parameters.iter();
	let mut arguments = function.arguments().iter().map(
		|x| crate::translate::common::choose_value_in_term(x, &parameters_iterator.next().unwrap(),
			context)
			.map(|x| Box::new(x)))
		.collect::<Result<Vec<_>, _>>()?;

	arguments.push(Box::new(predicate_literal));

	let and = foliage::Formula::and(arguments);

	Ok(foliage::Formula::exists(parameters, Box::new(and)))
}

pub(crate) fn translate_body_literal<C>(body_literal: &clingo::ast::BodyLiteral,
	context: &C)
	-> Result<foliage::Formula, crate::Error>
where
	C: crate::translate::common::GetOrCreateVariableDeclaration
		+ crate::translate::common::GetOrCreateFunctionDeclaration
		+ crate::translate::common::GetOrCreatePredicateDeclaration
		+ crate::translate::common::AssignVariableDeclarationDomain
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
			context),
		clingo::ast::LiteralType::Comparison(comparison) =>
		{
			let parameters = (0..2).map(|_|
				{
					let variable_declaration = std::rc::Rc::new(
						foliage::VariableDeclaration::new("<anonymous>".to_string()));
					context.assign_variable_declaration_domain(&variable_declaration,
						crate::translate::common::Domain::Program);
					variable_declaration
				})
				.collect::<foliage::VariableDeclarations>();
			let parameters = std::rc::Rc::new(parameters);

			let mut parameters_iterator = parameters.iter();
			let parameter_z1 = &parameters_iterator.next().unwrap();
			let parameter_z2 = &parameters_iterator.next().unwrap();

			let choose_z1_in_t1 = crate::translate::common::choose_value_in_term(comparison.left(),
				parameter_z1, context)?;
			let choose_z2_in_t2 = crate::translate::common::choose_value_in_term(comparison.right(),
				parameter_z2, context)?;

			let variable_1 = foliage::Term::variable(parameter_z1);
			let variable_2 = foliage::Term::variable(parameter_z2);

			let operator = crate::translate::common::translate_comparison_operator(
				comparison.comparison_type());

			let compare_z1_and_z2 = foliage::Formula::compare(operator, Box::new(variable_1),
				Box::new(variable_2));

			let and = foliage::Formula::and(vec![Box::new(choose_z1_in_t1),
				Box::new(choose_z2_in_t2), Box::new(compare_z1_and_z2)]);

			Ok(foliage::Formula::exists(parameters, Box::new(and)))
		},
		_ => Err(crate::Error::new_unsupported_language_feature(
			"body literals other than Booleans, terms, or comparisons")),
	}
}

pub(crate) fn translate_body<C>(body_literals: &[clingo::ast::BodyLiteral],
	context: &C)
	-> Result<foliage::Formulas, crate::Error>
where
	C: crate::translate::common::GetOrCreateVariableDeclaration
		+ crate::translate::common::GetOrCreateFunctionDeclaration
		+ crate::translate::common::GetOrCreatePredicateDeclaration
		+ crate::translate::common::AssignVariableDeclarationDomain
{
	body_literals.iter()
		.map(|body_literal| translate_body_literal(body_literal, context)
			.map(|value| Box::new(value)))
		.collect::<Result<foliage::Formulas, crate::Error>>()
}
