pub(crate) fn choose_value_in_primitive(term: Box<foliage::Term>,
	variable_declaration: std::rc::Rc<foliage::VariableDeclaration>)
	-> foliage::Formula
{
	let variable = foliage::Term::variable(variable_declaration);

	foliage::Formula::equal(Box::new(variable), term)
}

pub(crate) fn choose_value_in_term<C>(term: &clingo::ast::Term,
	variable_declaration: std::rc::Rc<foliage::VariableDeclaration>, context: &C)
	-> Result<foliage::Formula, crate::Error>
where
	C: crate::traits::GetOrCreateFunctionDeclaration
		+ crate::traits::GetOrCreateVariableDeclaration
		+ crate::traits::AssignVariableDeclarationDomain
{
	match term.term_type()
	{
		clingo::ast::TermType::Symbol(symbol) => match symbol.symbol_type()
			.map_err(|error| crate::Error::new_logic("clingo error").with(error))?
		{
			clingo::SymbolType::Number => Ok(choose_value_in_primitive(
				Box::new(foliage::Term::integer(symbol.number()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?)),
				variable_declaration)),
			clingo::SymbolType::Infimum => Ok(choose_value_in_primitive(
				Box::new(foliage::Term::infimum()), variable_declaration)),
			clingo::SymbolType::Supremum => Ok(choose_value_in_primitive(
				Box::new(foliage::Term::supremum()), variable_declaration)),
			clingo::SymbolType::String => Ok(choose_value_in_primitive(
				Box::new(foliage::Term::string(symbol.string()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?
					.to_string())),
				variable_declaration)),
			clingo::SymbolType::Function =>
			{
				let arguments = symbol.arguments()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?;

				// Functions with arguments are represented as clingo::ast::Function by the parser.
				// At this point, we only have to handle (0-ary) constants
				if !arguments.is_empty()
				{
					return Err(crate::Error::new_logic(
						"unexpected arguments, expected (0-ary) constant symbol"));
				}

				let constant_name = symbol.name()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?;

				let constant_declaration =
					context.get_or_create_function_declaration(constant_name, 0);
				let function = foliage::Term::function(constant_declaration, vec![]);

				Ok(choose_value_in_primitive(Box::new(function), variable_declaration))
			}
		},
		clingo::ast::TermType::Variable(variable_name) =>
		{
			let other_variable_declaration = context.get_or_create_variable_declaration(
				variable_name);
			context.assign_variable_declaration_domain(&other_variable_declaration,
				crate::Domain::Program);
			let other_variable = foliage::Term::variable(other_variable_declaration);

			Ok(choose_value_in_primitive(Box::new(other_variable), variable_declaration))
		},
		clingo::ast::TermType::BinaryOperation(binary_operation) =>
		{
			let operator = super::translate_binary_operator(binary_operation.binary_operator())?;

			match operator
			{
				foliage::BinaryOperator::Add
				| foliage::BinaryOperator::Subtract
				| foliage::BinaryOperator::Multiply
					=>
				{
					let parameters = (0..2).map(|_|
						{
							let variable_declaration = std::rc::Rc::new(
								foliage::VariableDeclaration::new("<anonymous>".to_string()));
							context.assign_variable_declaration_domain(&variable_declaration,
								crate::Domain::Integer);
							variable_declaration
						})
						.collect::<foliage::VariableDeclarations>();
					let parameters = std::rc::Rc::new(parameters);

					let parameter_1 = &parameters[0];
					let parameter_2 = &parameters[1];

					let translated_binary_operation = foliage::Term::binary_operation(operator,
						Box::new(foliage::Term::variable(std::rc::Rc::clone(&parameter_1))),
						Box::new(foliage::Term::variable(std::rc::Rc::clone(&parameter_2))));

					let equals = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(translated_binary_operation));

					let choose_value_from_left_argument = choose_value_in_term(
						binary_operation.left(), std::rc::Rc::clone(&parameter_1), context)?;

					let choose_value_from_right_argument = choose_value_in_term(
						binary_operation.right(), std::rc::Rc::clone(&parameter_2), context)?;

					let and = foliage::Formula::And(vec![equals, choose_value_from_left_argument,
						choose_value_from_right_argument]);

					Ok(foliage::Formula::exists(parameters, Box::new(and)))
				},
				foliage::BinaryOperator::Divide
				| foliage::BinaryOperator::Modulo
					=>
				{
					let parameters = (0..4).map(|_|
						{
							let variable_declaration = std::rc::Rc::new(
								foliage::VariableDeclaration::new("<anonymous>".to_string()));
							context.assign_variable_declaration_domain(&variable_declaration,
								crate::Domain::Integer);
							variable_declaration
						})
						.collect::<foliage::VariableDeclarations>();
					let parameters = std::rc::Rc::new(parameters);

					let parameter_i = &parameters[0];
					let parameter_j = &parameters[1];
					let parameter_q = &parameters[2];
					let parameter_r = &parameters[3];

					let j_times_q = foliage::Term::multiply(
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_j))),
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_q))));

					let j_times_q_plus_r = foliage::Term::add(Box::new(j_times_q),
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_r))));

					let i_equals_j_times_q_plus_r = foliage::Formula::equal(
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_j))),
						Box::new(j_times_q_plus_r));

					let choose_i_in_t1 = choose_value_in_term(binary_operation.left(),
						std::rc::Rc::clone(parameter_i), context)?;

					let choose_i_in_t2 = choose_value_in_term(binary_operation.left(),
						std::rc::Rc::clone(parameter_j), context)?;

					let j_not_equal_to_0 = foliage::Formula::not_equal(
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_j))),
						Box::new(foliage::Term::integer(0)));

					let r_greater_or_equal_to_0 = foliage::Formula::greater_or_equal(
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_r))),
						Box::new(foliage::Term::integer(0)));

					let r_less_than_q = foliage::Formula::less(
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_r))),
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_q))));

					let z_equal_to_q = foliage::Formula::equal(
						Box::new(
							foliage::Term::variable(std::rc::Rc::clone(&variable_declaration))),
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_q))));

					let z_equal_to_r = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_r))));

					let last_argument = match operator
					{
						foliage::BinaryOperator::Divide => z_equal_to_q,
						foliage::BinaryOperator::Modulo => z_equal_to_r,
						_ => return Err(crate::Error::new_logic("unreachable code")),
					};

					let and = foliage::Formula::and(vec![i_equals_j_times_q_plus_r, choose_i_in_t1,
						choose_i_in_t2, j_not_equal_to_0, r_greater_or_equal_to_0, r_less_than_q,
						last_argument]);

					Ok(foliage::Formula::exists(parameters, Box::new(and)))
				},
				foliage::BinaryOperator::Exponentiate =>
					Err(crate::Error::new_unsupported_language_feature("exponentiation")),
			}
		},
		clingo::ast::TermType::UnaryOperation(unary_operation) =>
		{
			match unary_operation.unary_operator()
			{
				clingo::ast::UnaryOperator::Absolute =>
					return Err(crate::Error::new_unsupported_language_feature("absolute value")),
				clingo::ast::UnaryOperator::Minus =>
				{
					let parameter_z_prime = std::rc::Rc::new(foliage::VariableDeclaration::new(
						"<anonymous>".to_string()));
					context.assign_variable_declaration_domain(&parameter_z_prime,
						crate::Domain::Integer);

					let negative_z_prime = foliage::Term::negative(Box::new(
						foliage::Term::variable(std::rc::Rc::clone(&parameter_z_prime))));
					let equals = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(negative_z_prime));

					let choose_z_prime_in_t_prime = choose_value_in_term(unary_operation.argument(),
						std::rc::Rc::clone(&parameter_z_prime), context)?;

					let and = foliage::Formula::and(vec![equals, choose_z_prime_in_t_prime]);

					let parameters = std::rc::Rc::new(vec![parameter_z_prime]);

					Ok(foliage::Formula::exists(parameters, Box::new(and)))
				},
				_ => Err(crate::Error::new_not_yet_implemented("todo")),
			}
		},
		clingo::ast::TermType::Interval(interval) =>
		{
			let parameters = (0..3).map(|_|
				{
					let variable_declaration = std::rc::Rc::new(
						foliage::VariableDeclaration::new("<anonymous>".to_string()));
					context.assign_variable_declaration_domain(&variable_declaration,
						crate::Domain::Integer);
					variable_declaration
				})
				.collect::<foliage::VariableDeclarations>();
			let parameters = std::rc::Rc::new(parameters);

			let parameter_i = &parameters[0];
			let parameter_j = &parameters[1];
			let parameter_k = &parameters[2];

			let choose_i_in_t_1 = choose_value_in_term(interval.left(),
				std::rc::Rc::clone(parameter_i), context)?;

			let choose_j_in_t_2 = choose_value_in_term(interval.right(),
				std::rc::Rc::clone(parameter_j), context)?;

			let i_less_than_or_equal_to_k = foliage::Formula::less_or_equal(
				Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_i))),
				Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_k))));

			let k_less_than_or_equal_to_j = foliage::Formula::less_or_equal(
				Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_k))),
				Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_j))));

			let z_equals_k = foliage::Formula::equal(
				Box::new(foliage::Term::variable(variable_declaration)),
				Box::new(foliage::Term::variable(std::rc::Rc::clone(parameter_k))));

			let and = foliage::Formula::and(vec![choose_i_in_t_1, choose_j_in_t_2,
				i_less_than_or_equal_to_k, k_less_than_or_equal_to_j, z_equals_k]);

			Ok(foliage::Formula::exists(parameters, Box::new(and)))
		},
		clingo::ast::TermType::Function(_) =>
			Err(crate::Error::new_unsupported_language_feature("symbolic functions")),
		clingo::ast::TermType::Pool(_) =>
			Err(crate::Error::new_unsupported_language_feature("pools")),
		clingo::ast::TermType::ExternalFunction(_) =>
			Err(crate::Error::new_unsupported_language_feature("external functions")),
	}
}
