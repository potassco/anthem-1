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
				let declaration = foliage::FunctionDeclaration
				{
					name: name.to_owned(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

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
				let declaration = foliage::PredicateDeclaration
				{
					name: name.to_owned(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

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
			let variable_declaration = foliage::VariableDeclaration
			{
				name: "_".to_owned(),
			};
			let variable_declaration = std::rc::Rc::new(variable_declaration);

			self.free_variable_declarations.insert(
				std::rc::Rc::clone(&variable_declaration));

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
			=> Ok(foliage::BinaryOperator::Addition),
		clingo::ast::BinaryOperator::Minus
			=> Ok(foliage::BinaryOperator::Subtraction),
		clingo::ast::BinaryOperator::Multiplication
			=> Ok(foliage::BinaryOperator::Multiplication),
		clingo::ast::BinaryOperator::Division
			=> Ok(foliage::BinaryOperator::Division),
		clingo::ast::BinaryOperator::Modulo
			=> Ok(foliage::BinaryOperator::Modulo),
		clingo::ast::BinaryOperator::Power
			=> Ok(foliage::BinaryOperator::Exponentiation),
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
	-> foliage::Comparison
{
	let variable = foliage::Variable
	{
		declaration: std::rc::Rc::clone(variable_declaration),
	};

	foliage::Comparison
	{
		operator: foliage::ComparisonOperator::Equal,
		left: Box::new(foliage::Term::Variable(variable)),
		right: term,
	}
}

pub(crate) fn choose_value_in_term(term: &clingo::ast::Term,
	variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>,
	function_declarations: &mut foliage::FunctionDeclarations,
	variable_declaration_stack: &mut foliage::VariableDeclarationStack)
	-> Result<foliage::Formula, crate::Error>
{
	match term.term_type()
	{
		clingo::ast::TermType::Symbol(symbol) => match symbol.symbol_type()
			.map_err(|error| crate::Error::new_logic("clingo error").with(error))?
		{
			clingo::SymbolType::Number => Ok(foliage::Formula::Comparison(choose_value_in_primitive(
				Box::new(foliage::Term::Integer(symbol.number()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?)),
				variable_declaration))),
			clingo::SymbolType::Infimum => Ok(foliage::Formula::Comparison(choose_value_in_primitive(
				Box::new(foliage::Term::SpecialInteger(foliage::SpecialInteger::Infimum)),
				variable_declaration))),
			clingo::SymbolType::Supremum => Ok(foliage::Formula::Comparison(choose_value_in_primitive(
				Box::new(foliage::Term::SpecialInteger(foliage::SpecialInteger::Supremum)),
				variable_declaration))),
			clingo::SymbolType::String => Ok(foliage::Formula::Comparison(choose_value_in_primitive(
				Box::new(foliage::Term::String(symbol.string()
					.map_err(|error| crate::Error::new_logic("clingo error").with(error))?
					.to_string())),
				variable_declaration))),
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

				let function = foliage::Term::Function(foliage::Function
				{
					declaration: constant_declaration,
					arguments: vec![],
				});

				Ok(foliage::Formula::Comparison(choose_value_in_primitive(Box::new(function), variable_declaration)))
			}
		},
		clingo::ast::TermType::Variable(variable_name) =>
		{
			let other_variable_declaration = variable_declaration_stack.find_or_create(variable_name);

			let other_variable = foliage::Term::Variable(foliage::Variable
			{
				declaration: other_variable_declaration,
			});

			Ok(foliage::Formula::Comparison(choose_value_in_primitive(Box::new(other_variable),
				variable_declaration)))
		},
		_ => Err(crate::Error::new_not_yet_implemented("todo")),
	}
}
