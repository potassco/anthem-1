pub(crate) trait FindOrCreateFunctionDeclaration = FnMut(&str, usize)
	-> std::rc::Rc<foliage::FunctionDeclaration>;

pub(crate) trait FindOrCreatePredicateDeclaration = FnMut(&str, usize)
	-> std::rc::Rc<foliage::PredicateDeclaration>;

pub(crate) trait FindOrCreateVariableDeclaration = FnMut(&str)
	-> std::rc::Rc<foliage::VariableDeclaration>;

pub(crate) fn find_or_create_predicate_declaration(predicate_declarations: &mut foliage::PredicateDeclarations,
	name: &str, arity: usize)
	-> std::rc::Rc<foliage::PredicateDeclaration>
{
	match predicate_declarations.iter()
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

			predicate_declarations.insert(std::rc::Rc::clone(&declaration));

			log::debug!("new predicate declaration: {}/{}", name, arity);

			declaration
		},
	}
}

pub(crate) fn find_or_create_function_declaration(function_declarations: &mut foliage::FunctionDeclarations,
	name: &str, arity: usize)
	-> std::rc::Rc<foliage::FunctionDeclaration>
{
	match function_declarations.iter()
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

			function_declarations.insert(std::rc::Rc::clone(&declaration));

			log::debug!("new function declaration: {}/{}", name, arity);

			declaration
		},
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

pub(crate) fn choose_value_in_term<F1, F2>(term: &clingo::ast::Term,
	variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>,
	mut find_or_create_function_declaration: F1, mut find_or_create_variable_declaration: F2)
	-> Result<foliage::Formula, crate::Error>
where
	F1: FindOrCreateFunctionDeclaration,
	F2: FindOrCreateVariableDeclaration,
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

				let constant_declaration = find_or_create_function_declaration(constant_name, 0);

				let function = foliage::Term::Function(foliage::Function
				{
					declaration: constant_declaration,
					arguments: vec![],
				});

				Ok(foliage::Formula::Comparison(choose_value_in_primitive(Box::new(function), variable_declaration)))
			}
		},
		clingo::ast::TermType::Variable(variable) =>
			Err(crate::Error::new_not_yet_implemented("todo")),
		_ => Err(crate::Error::new_not_yet_implemented("todo")),
	}
}
