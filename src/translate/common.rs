pub(crate) trait FindOrCreateFunctionDeclaration
{
	fn find_or_create(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>;
}

pub(crate) trait FindOrCreatePredicateDeclaration
{
	fn find_or_create(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>;
}

pub(crate) trait FindOrCreateVariableDeclaration
{
	fn find_or_create(&mut self, name: &str)
		-> std::rc::Rc<foliage::VariableDeclaration>;
}

impl FindOrCreateFunctionDeclaration for foliage::FunctionDeclarations
{
	fn find_or_create(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		match self.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::FunctionDeclaration::new(
					name.to_string(), arity));

				self.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl FindOrCreatePredicateDeclaration for foliage::PredicateDeclarations
{
	fn find_or_create(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		match self.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::PredicateDeclaration::new(
					name.to_string(), arity));

				self.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl FindOrCreateVariableDeclaration for foliage::VariableDeclarationStack
{
	fn find_or_create(&mut self, name: &str)
		-> std::rc::Rc<foliage::VariableDeclaration>
	{
		// TODO: check correctness
		if name == "_"
		{
			let variable_declaration = std::rc::Rc::new(foliage::VariableDeclaration::new(
				"_".to_owned()));

			self.free_variable_declarations.push(std::rc::Rc::clone(&variable_declaration));

			return variable_declaration;
		}

		self.find_or_create(name)
	}
}

pub(crate) fn translate_binary_operator(binary_operator: clingo::ast::BinaryOperator)
	-> Result<foliage::BinaryOperator, crate::Error>
{
	match binary_operator
	{
		clingo::ast::BinaryOperator::And
		| clingo::ast::BinaryOperator::Or
		| clingo::ast::BinaryOperator::Xor
			=> return Err(crate::Error::new_unsupported_language_feature("binary logical operators")),
		clingo::ast::BinaryOperator::Plus
			=> Ok(foliage::BinaryOperator::Add),
		clingo::ast::BinaryOperator::Minus
			=> Ok(foliage::BinaryOperator::Subtract),
		clingo::ast::BinaryOperator::Multiplication
			=> Ok(foliage::BinaryOperator::Multiply),
		clingo::ast::BinaryOperator::Division
			=> Ok(foliage::BinaryOperator::Divide),
		clingo::ast::BinaryOperator::Modulo
			=> Ok(foliage::BinaryOperator::Modulo),
		clingo::ast::BinaryOperator::Power
			=> Ok(foliage::BinaryOperator::Exponentiate),
	}
}

pub(crate) fn translate_comparison_operator(comparison_operator: clingo::ast::ComparisonOperator)
	-> foliage::ComparisonOperator
{
	match comparison_operator
	{
		clingo::ast::ComparisonOperator::GreaterThan
			=> foliage::ComparisonOperator::Greater,
		clingo::ast::ComparisonOperator::LessThan
			=> foliage::ComparisonOperator::Less,
		clingo::ast::ComparisonOperator::LessEqual
			=> foliage::ComparisonOperator::LessOrEqual,
		clingo::ast::ComparisonOperator::GreaterEqual
			=> foliage::ComparisonOperator::GreaterOrEqual,
		clingo::ast::ComparisonOperator::NotEqual
			=> foliage::ComparisonOperator::NotEqual,
		clingo::ast::ComparisonOperator::Equal
			=> foliage::ComparisonOperator::Equal,
	}
}

pub(crate) fn choose_value_in_primitive(term: Box<foliage::Term>,
	variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
	-> foliage::Formula
{
	let variable = foliage::Term::variable(variable_declaration);

	foliage::Formula::equal(Box::new(variable), term)
}

pub(crate) fn choose_value_in_term(term: &clingo::ast::Term,
	variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>,
	mut function_declarations: &mut foliage::FunctionDeclarations,
	mut variable_declaration_stack: &mut foliage::VariableDeclarationStack)
	-> Result<foliage::Formula, crate::Error>
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
					return Err(crate::Error::new_logic("unexpected arguments, expected (0-ary) constant symbol"));
				}

				let constant_name = symbol.name()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?;

				let constant_declaration = function_declarations.find_or_create(constant_name, 0);
				let function = foliage::Term::function(&constant_declaration, vec![]);

				Ok(choose_value_in_primitive(Box::new(function), variable_declaration))
			}
		},
		clingo::ast::TermType::Variable(variable_name) =>
		{
			let other_variable_declaration = variable_declaration_stack.find_or_create(variable_name);
			let other_variable = foliage::Term::variable(&other_variable_declaration);

			Ok(choose_value_in_primitive(Box::new(other_variable), variable_declaration))
		},
		clingo::ast::TermType::BinaryOperation(binary_operation) =>
		{
			let operator = crate::translate::common::translate_binary_operator(
				binary_operation.binary_operator())?;

			match operator
			{
				foliage::BinaryOperator::Add
				| foliage::BinaryOperator::Subtract
				| foliage::BinaryOperator::Multiply
					=>
				{
					let parameters = (0..2).map(|_| std::rc::Rc::new(
							foliage::VariableDeclaration::new("<anonymous>".to_string())))
						.collect::<foliage::VariableDeclarations>();

					let parameter_1 = &parameters[0];
					let parameter_2 = &parameters[1];

					let translated_binary_operation = foliage::Term::binary_operation(operator,
						Box::new(foliage::Term::variable(&parameter_1)),
						Box::new(foliage::Term::variable(&parameter_2)));

					let equals = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(translated_binary_operation));

					let choose_value_from_left_argument = choose_value_in_term(
						binary_operation.left(), &parameter_1, &mut function_declarations,
						&mut variable_declaration_stack)?;

					let choose_value_from_right_argument = choose_value_in_term(
						binary_operation.right(), &parameter_2, &mut function_declarations,
						&mut variable_declaration_stack)?;

					let and = foliage::Formula::And(vec![Box::new(equals),
						Box::new(choose_value_from_left_argument),
						Box::new(choose_value_from_right_argument)]);

					Ok(foliage::Formula::exists(parameters, Box::new(and)))
				},
				foliage::BinaryOperator::Divide
				| foliage::BinaryOperator::Modulo
					=>
				{
					let parameters = (0..4).map(|_| std::rc::Rc::new(
							foliage::VariableDeclaration::new("<anonymous>".to_string())))
						.collect::<foliage::VariableDeclarations>();

					let parameter_i = &parameters[0];
					let parameter_j = &parameters[1];
					let parameter_q = &parameters[2];
					let parameter_r = &parameters[3];

					let j_times_q = foliage::Term::multiply(
						Box::new(foliage::Term::variable(parameter_j)),
						Box::new(foliage::Term::variable(parameter_q)));

					let j_times_q_plus_r = foliage::Term::add(Box::new(j_times_q),
						Box::new(foliage::Term::variable(parameter_r)));

					let i_equals_j_times_q_plus_r = foliage::Formula::equal(
						Box::new(foliage::Term::variable(parameter_j)), Box::new(j_times_q_plus_r));

					let choose_i_in_t1 = choose_value_in_term(binary_operation.left(), parameter_i,
						&mut function_declarations, &mut variable_declaration_stack)?;

					let choose_i_in_t2 = choose_value_in_term(binary_operation.left(), parameter_j,
						&mut function_declarations, &mut variable_declaration_stack)?;

					let j_not_equal_to_0 = foliage::Formula::not_equal(
						Box::new(foliage::Term::variable(parameter_j)),
						Box::new(foliage::Term::integer(0)));

					let r_greater_or_equal_to_0 = foliage::Formula::greater_or_equal(
						Box::new(foliage::Term::variable(parameter_r)),
						Box::new(foliage::Term::integer(0)));

					let r_less_than_q = foliage::Formula::less(
						Box::new(foliage::Term::variable(parameter_r)),
						Box::new(foliage::Term::variable(parameter_q)));

					let z_equal_to_q = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(foliage::Term::variable(parameter_q)));

					let z_equal_to_r = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(foliage::Term::variable(parameter_r)));

					let last_argument = match operator
					{
						foliage::BinaryOperator::Divide => z_equal_to_q,
						foliage::BinaryOperator::Modulo => z_equal_to_r,
						_ => return Err(crate::Error::new_logic("unreachable code")),
					};

					let and = foliage::Formula::and(vec![Box::new(i_equals_j_times_q_plus_r),
						Box::new(choose_i_in_t1), Box::new(choose_i_in_t2),
						Box::new(j_not_equal_to_0), Box::new(r_greater_or_equal_to_0),
						Box::new(r_less_than_q), Box::new(last_argument)]);

					Ok(foliage::Formula::exists(parameters, Box::new(and)))
				},
				_ => Err(crate::Error::new_not_yet_implemented("todo")),
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

					let negative_z_prime = foliage::Term::negative(Box::new(
						foliage::Term::variable(&parameter_z_prime)));
					let equals = foliage::Formula::equal(
						Box::new(foliage::Term::variable(variable_declaration)),
						Box::new(negative_z_prime));

					let choose_z_prime_in_t_prime = choose_value_in_term(unary_operation.argument(),
						&parameter_z_prime, &mut function_declarations,
						&mut variable_declaration_stack)?;

					let and = foliage::Formula::and(vec![Box::new(equals),
						Box::new(choose_z_prime_in_t_prime)]);

					let parameters = vec![parameter_z_prime];

					Ok(foliage::Formula::exists(parameters, Box::new(and)))
				},
				_ => Err(crate::Error::new_not_yet_implemented("todo")),
			}
		},
		clingo::ast::TermType::Interval(interval) =>
		{
			let parameters = (0..3).map(|_| std::rc::Rc::new(
					foliage::VariableDeclaration::new("<anonymous>".to_string())))
				.collect::<foliage::VariableDeclarations>();

			let parameter_i = &parameters[0];
			let parameter_j = &parameters[1];
			let parameter_k = &parameters[2];

			let choose_i_in_t_1 = choose_value_in_term(interval.left(), parameter_i,
				&mut function_declarations, &mut variable_declaration_stack)?;

			let choose_j_in_t_2 = choose_value_in_term(interval.right(), parameter_j,
				&mut function_declarations, &mut variable_declaration_stack)?;

			let i_less_than_or_equal_to_k = foliage::Formula::less_or_equal(
				Box::new(foliage::Term::variable(parameter_i)),
				Box::new(foliage::Term::variable(parameter_k)));

			let k_less_than_or_equal_to_j = foliage::Formula::less_or_equal(
				Box::new(foliage::Term::variable(parameter_k)),
				Box::new(foliage::Term::variable(parameter_j)));

			let z_equals_k = foliage::Formula::equal(
				Box::new(foliage::Term::variable(variable_declaration)),
				Box::new(foliage::Term::variable(parameter_k)));

			let and = foliage::Formula::and(vec![Box::new(choose_i_in_t_1),
				Box::new(choose_j_in_t_2), Box::new(i_less_than_or_equal_to_k),
				Box::new(k_less_than_or_equal_to_j), Box::new(z_equals_k)]);

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
