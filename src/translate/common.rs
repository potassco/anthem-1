pub fn translate_comparison_operator(comparison_operator: clingo::ast::ComparisonOperator)
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

pub fn choose_value_in_primitive(term: Box<foliage::Term>,
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

pub fn choose_value_in_term<F>(term: &clingo::ast::Term,
	variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>,
	mut find_or_create_function_declaration: F)
	-> Result<foliage::Formula, crate::Error>
where
	F: FnMut(&str, usize) -> std::rc::Rc<foliage::FunctionDeclaration>
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
		_ => Ok(foliage::Formula::Boolean(false))
	}
}
