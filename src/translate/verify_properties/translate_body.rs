// TODO: rename context
pub(crate) fn translate_body_term<C>(body_term: &clingo::ast::Term, sign: clingo::ast::Sign,
	context: &C, variable_declaration_stack: &crate::VariableDeclarationStackLayer,
	head_predicate_dependencies: &mut Option<&mut crate::PredicateDependencies>)
	-> Result<crate::Formula, crate::Error>
where
	C: foliage::FindOrCreateFunctionDeclaration<crate::FoliageFlavor>
		+ foliage::FindOrCreatePredicateDeclaration<crate::FoliageFlavor>,
{
	let function = match body_term.term_type()
	{
		clingo::ast::TermType::Function(value) => value,
		_ => return Err(crate::Error::new_unsupported_language_feature("only functions supported as body terms")),
	};

	let function_name = function.name().map_err(|error| crate::Error::new_decode_identifier(error))?;

	let predicate_declaration = context.find_or_create_predicate_declaration(function_name,
		function.arguments().len());

	let parameters = function.arguments().iter().map(
		|_| std::rc::Rc::new(crate::VariableDeclaration::new_generated(crate::Domain::Program)))
		.collect::<crate::VariableDeclarations>();
	let parameters = std::rc::Rc::new(parameters);

	let predicate_arguments = parameters.iter().map(
		|parameter| crate::Term::variable(std::rc::Rc::clone(parameter)))
		.collect::<Vec<_>>();

	if let Some(head_predicate_dependencies) = head_predicate_dependencies
	{
		match head_predicate_dependencies.get_mut(&predicate_declaration)
		{
			None =>
			{
				let predicate_dependency_sign = match sign
				{
					clingo::ast::Sign::None
					| clingo::ast::Sign::DoubleNegation => crate::PredicateDependencySign::OnlyPositive,
					clingo::ast::Sign::Negation => crate::PredicateDependencySign::OnlyNegative,
				};

				head_predicate_dependencies.insert(
					std::rc::Rc::clone(&predicate_declaration), predicate_dependency_sign);
			},
			Some(predicate_dependency_sign) =>
				*predicate_dependency_sign = match sign
				{
					clingo::ast::Sign::None
					| clingo::ast::Sign::DoubleNegation => predicate_dependency_sign.and_positive(),
					clingo::ast::Sign::Negation => predicate_dependency_sign.and_negative(),
				},
		}
	}

	let predicate = crate::Formula::predicate(predicate_declaration, predicate_arguments);

	let predicate_literal = match sign
	{
		clingo::ast::Sign::None
		| clingo::ast::Sign::DoubleNegation
			=> predicate,
		clingo::ast::Sign::Negation
			=> crate::Formula::not(Box::new(predicate)),
	};

	if function.arguments().is_empty()
	{
		return Ok(predicate_literal);
	}

	assert_eq!(function.arguments().len(), parameters.len());

	let mut parameters_iterator = parameters.iter();
	let mut arguments = function.arguments().iter().map(
		|x| crate::translate::common::choose_value_in_term(x,
			std::rc::Rc::clone(&parameters_iterator.next().unwrap()), context,
			variable_declaration_stack))
		.collect::<Result<Vec<_>, _>>()?;

	arguments.push(predicate_literal);

	let and = crate::Formula::and(arguments);

	Ok(crate::Formula::exists(parameters, Box::new(and)))
}

pub(crate) fn translate_body_literal<C>(body_literal: &clingo::ast::BodyLiteral,
	context: &C, variable_declaration_stack: &crate::VariableDeclarationStackLayer,
	head_predicate_dependencies: &mut Option<&mut crate::PredicateDependencies>)
	-> Result<crate::Formula, crate::Error>
where
	C: foliage::FindOrCreateFunctionDeclaration<crate::FoliageFlavor>
		+ foliage::FindOrCreatePredicateDeclaration<crate::FoliageFlavor>,
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

			Ok(crate::Formula::Boolean(value))
		},
		clingo::ast::LiteralType::Symbolic(term) => translate_body_term(term, literal.sign(),
			context, variable_declaration_stack, head_predicate_dependencies),
		clingo::ast::LiteralType::Comparison(comparison) =>
		{
			let parameters = (0..2).map(
				|_| std::rc::Rc::new(crate::VariableDeclaration::new_generated(
					crate::Domain::Program)))
				.collect::<crate::VariableDeclarations>();
			let parameters = std::rc::Rc::new(parameters);

			let mut parameters_iterator = parameters.iter();
			let parameter_z1 = &parameters_iterator.next().unwrap();
			let parameter_z2 = &parameters_iterator.next().unwrap();

			let choose_z1_in_t1 = crate::translate::common::choose_value_in_term(comparison.left(),
				std::rc::Rc::clone(parameter_z1), context, variable_declaration_stack)?;
			let choose_z2_in_t2 = crate::translate::common::choose_value_in_term(comparison.right(),
				std::rc::Rc::clone(parameter_z2), context, variable_declaration_stack)?;

			let variable_1 = crate::Term::variable(std::rc::Rc::clone(parameter_z1));
			let variable_2 = crate::Term::variable(std::rc::Rc::clone(parameter_z2));

			let operator = crate::translate::common::translate_comparison_operator(
				comparison.comparison_type());

			let compare_z1_and_z2 = crate::Formula::compare(operator, Box::new(variable_1),
				Box::new(variable_2));

			let and =
				crate::Formula::and(vec![choose_z1_in_t1, choose_z2_in_t2, compare_z1_and_z2]);

			Ok(crate::Formula::exists(parameters, Box::new(and)))
		},
		_ => Err(crate::Error::new_unsupported_language_feature(
			"body literals other than Booleans, terms, or comparisons")),
	}
}

pub(crate) fn translate_body<C>(body_literals: &[clingo::ast::BodyLiteral], context: &C,
	variable_declaration_stack: &crate::VariableDeclarationStackLayer,
	// TODO: refactor
	mut head_predicate_dependencies: &mut Option<&mut crate::PredicateDependencies>)
	-> Result<crate::Formulas, crate::Error>
where
	C: foliage::FindOrCreateFunctionDeclaration<crate::FoliageFlavor>
		+ foliage::FindOrCreatePredicateDeclaration<crate::FoliageFlavor>,
{
	body_literals.iter()
		.map(|body_literal| translate_body_literal(body_literal, context,
			variable_declaration_stack, &mut head_predicate_dependencies))
		.collect::<Result<crate::Formulas, crate::Error>>()
}
