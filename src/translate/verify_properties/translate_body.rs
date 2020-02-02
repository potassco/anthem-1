pub(crate) fn translate_body_term(body_term: &clingo::ast::Term, sign: clingo::ast::Sign,
	mut function_declarations: &mut foliage::FunctionDeclarations,
	predicate_declarations: &mut foliage::PredicateDeclarations,
	mut variable_declaration_stack: &mut foliage::VariableDeclarationStack)
	-> Result<foliage::Formula, crate::Error>
{
	use crate::translate::common::FindOrCreatePredicateDeclaration;

	let function = match body_term.term_type()
	{
		clingo::ast::TermType::Function(value) => value,
		_ => return Err(crate::Error::new_unsupported_language_feature("only functions supported as body terms")),
	};

	let function_name = function.name().map_err(|error| crate::Error::new_decode_identifier(error))?;

	let predicate_declaration = predicate_declarations.find_or_create(function_name,
		function.arguments().len());

	let parameters = function.arguments().iter().map(|_| std::rc::Rc::new(
		foliage::VariableDeclaration::new("<anonymous>".to_string())))
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
				&mut function_declarations, &mut variable_declaration_stack)
			.map(|x| Box::new(x)))
		.collect::<Result<Vec<_>, _>>()?;

	arguments.push(Box::new(predicate_literal));

	let and = foliage::Formula::and(arguments);

	Ok(foliage::Formula::exists(parameters, Box::new(and)))
}

pub(crate) fn translate_body_literal(body_literal: &clingo::ast::BodyLiteral,
	mut function_declarations: &mut foliage::FunctionDeclarations,
	mut predicate_declarations: &mut foliage::PredicateDeclarations,
	mut variable_declaration_stack: &mut foliage::VariableDeclarationStack)
	-> Result<foliage::Formula, crate::Error>
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
			&mut function_declarations, &mut predicate_declarations,
			&mut variable_declaration_stack),
		clingo::ast::LiteralType::Comparison(comparison) =>
		{
			let parameters = (0..2).map(|_| std::rc::Rc::new(foliage::VariableDeclaration::new(
					"<anonymous>".to_string())))
				.collect::<foliage::VariableDeclarations>();
			let parameters = std::rc::Rc::new(parameters);

			let mut parameters_iterator = parameters.iter();
			let parameter_z1 = &parameters_iterator.next().unwrap();
			let parameter_z2 = &parameters_iterator.next().unwrap();

			let choose_z1_in_t1 = crate::translate::common::choose_value_in_term(comparison.left(),
				parameter_z1, &mut function_declarations, &mut variable_declaration_stack)?;
			let choose_z2_in_t2 = crate::translate::common::choose_value_in_term(comparison.right(),
				parameter_z2, &mut function_declarations, &mut variable_declaration_stack)?;

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

pub(crate) fn translate_body(body_literals: &[clingo::ast::BodyLiteral],
	mut function_declaration: &mut foliage::FunctionDeclarations,
	mut predicate_declaration: &mut foliage::PredicateDeclarations,
	mut variable_declaration_stack: &mut foliage::VariableDeclarationStack)
	-> Result<foliage::Formulas, crate::Error>
{
	body_literals.iter()
		.map(|body_literal| translate_body_literal(body_literal, &mut function_declaration,
			&mut predicate_declaration, &mut variable_declaration_stack)
			.map(|value| Box::new(value)))
		.collect::<Result<foliage::Formulas, crate::Error>>()
}
