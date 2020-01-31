pub struct ScopedFormula
{
	free_variable_declarations: foliage::VariableDeclarations,
	formula: foliage::Formula,
}

pub struct Context
{
	scoped_formulas: Vec<ScopedFormula>,
	function_declarations: foliage::FunctionDeclarations,
	predicate_declarations: foliage::PredicateDeclarations,
	variable_declaration_stack: foliage::VariableDeclarationStack,
}

impl Context
{
	pub fn new() -> Self
	{
		Self
		{
			scoped_formulas: vec![],
			function_declarations: foliage::FunctionDeclarations::new(),
			predicate_declarations: foliage::PredicateDeclarations::new(),
			variable_declaration_stack: foliage::VariableDeclarationStack::new(),
		}
	}

	pub fn find_or_create_predicate_declaration(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		match self.predicate_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = foliage::PredicateDeclaration
				{
					name: name.to_owned(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

				self.predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}

	pub fn find_or_create_function_declaration(&mut self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		match self.function_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = foliage::FunctionDeclaration
				{
					name: name.to_owned(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

				self.function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

pub fn translate_body_term(body_term: &clingo::ast::Term, sign: clingo::ast::Sign,
	context: &mut Context)
	-> Result<foliage::Formula, crate::Error>
{
	let function = match body_term.term_type()
	{
		clingo::ast::TermType::Function(value) => value,
		_ => return Err(crate::Error::new_unsupported_language_feature("only functions supported as body terms")),
	};

	let function_name = function.name().map_err(|error| crate::Error::new_decode_identifier(error))?;

	let predicate_declaration = context.find_or_create_predicate_declaration(function_name,
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
			let result = super::common::choose_value_in_term(x, &parameters[i],
				|name, arity| context.find_or_create_function_declaration(name, arity))
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

pub fn translate_body_literal(body_literal: &clingo::ast::BodyLiteral, context: &mut Context)
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
		clingo::ast::LiteralType::Symbolic(term) => translate_body_term(term, literal.sign(), context),
		clingo::ast::LiteralType::Comparison(comparison) =>
		{
			let new_variable_declaration = || std::rc::Rc::new(foliage::VariableDeclaration
			{
				name: "<anonymous>".to_string()
			});

			let parameters = vec![new_variable_declaration(), new_variable_declaration()];

			let parameter_z1 = &parameters[0];
			let parameter_z2 = &parameters[1];

			let choose_z1_in_t1 = super::common::choose_value_in_term(comparison.left(), &parameter_z1,
				|name, arity| context.find_or_create_function_declaration(name, arity))?;
			let choose_z2_in_t2 = super::common::choose_value_in_term(comparison.right(), &parameter_z2,
				|name, arity| context.find_or_create_function_declaration(name, arity))?;

			let variable_1 = foliage::Variable
			{
				declaration: std::rc::Rc::clone(&parameter_z1),
			};

			let variable_2 = foliage::Variable
			{
				declaration: std::rc::Rc::clone(&parameter_z2),
			};

			let comparison_operator
				= super::common::translate_comparison_operator(comparison.comparison_type());

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

pub fn translate_body(body_literals: &[clingo::ast::BodyLiteral], context: &mut Context)
	-> Result<foliage::Formulas, crate::Error>
{
	body_literals.iter()
		.map(|body_literal| translate_body_literal(body_literal, context)
			.map(|value| Box::new(value)))
		.collect::<Result<foliage::Formulas, crate::Error>>()
}

pub fn read(rule: &clingo::ast::Rule, context: &mut Context)
{
	println!("{:?}", translate_body(rule.body(), context));
}
