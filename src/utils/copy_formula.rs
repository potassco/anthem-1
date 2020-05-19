fn replace_variables_in_term(term: &mut foliage::Term,
	old_variable_declarations: &foliage::VariableDeclarations,
	new_variable_declarations: &foliage::VariableDeclarations)
{
	assert_eq!(old_variable_declarations.len(), new_variable_declarations.len());

	use foliage::Term;

	match term
	{
		Term::BinaryOperation(binary_operation) =>
		{
			replace_variables_in_term(&mut binary_operation.left, old_variable_declarations,
				new_variable_declarations);
			replace_variables_in_term(&mut binary_operation.right, old_variable_declarations,
				new_variable_declarations);
		},
		Term::Function(function) =>
			for mut argument in &mut function.arguments
			{
				replace_variables_in_term(&mut argument, old_variable_declarations,
					new_variable_declarations);
			},
		Term::UnaryOperation(unary_operation) =>
			replace_variables_in_term(&mut unary_operation.argument, old_variable_declarations,
				new_variable_declarations),
		Term::Variable(variable) =>
			if let Some(index) = old_variable_declarations.iter().enumerate()
				.find_map(
					|(index, variable_declaration)|
					{
						match *variable_declaration == variable.declaration
						{
							true => Some(index),
							false => None,
						}
					})
			{
				variable.declaration = std::rc::Rc::clone(&new_variable_declarations[index]);
			},
		Term::Boolean(_)
		| Term::Integer(_)
		| Term::SpecialInteger(_)
		| Term::String(_) => (),
	}
}

fn replace_variables_in_formula(formula: &mut foliage::Formula,
	old_variable_declarations: &foliage::VariableDeclarations,
	new_variable_declarations: &foliage::VariableDeclarations)
{
	use foliage::Formula;

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
			for argument in arguments
			{
				replace_variables_in_formula(argument, old_variable_declarations,
					new_variable_declarations);
			},
		Formula::Compare(compare) =>
		{
			replace_variables_in_term(&mut compare.left, old_variable_declarations,
				new_variable_declarations);
			replace_variables_in_term(&mut compare.right, old_variable_declarations,
				new_variable_declarations);
		},
		Formula::Exists(quantified_formula)
		| Formula::ForAll(quantified_formula) =>
			replace_variables_in_formula(&mut quantified_formula.argument,
				old_variable_declarations, new_variable_declarations),
		Formula::Implies(implies) =>
		{
			replace_variables_in_formula(&mut implies.antecedent, old_variable_declarations,
				new_variable_declarations);
			replace_variables_in_formula(&mut implies.implication, old_variable_declarations,
				new_variable_declarations);
		},
		Formula::Not(argument) =>
			replace_variables_in_formula(argument, old_variable_declarations,
				new_variable_declarations),
		Formula::Predicate(predicate) =>
			for mut argument in &mut predicate.arguments
			{
				replace_variables_in_term(&mut argument, old_variable_declarations,
					new_variable_declarations);
			},
		Formula::Boolean(_) => (),
	}
}

fn replace_variable_in_term_with_term(term: &mut foliage::Term,
	variable_declaration: &foliage::VariableDeclaration, replacement_term: &foliage::Term)
{
	use foliage::Term;

	match term
	{
		Term::BinaryOperation(binary_operation) =>
		{
			replace_variable_in_term_with_term(&mut binary_operation.left, variable_declaration,
				replacement_term);
			replace_variable_in_term_with_term(&mut binary_operation.right, variable_declaration,
				replacement_term);
		},
		Term::Function(function) =>
			for mut argument in &mut function.arguments
			{
				replace_variable_in_term_with_term(&mut argument, variable_declaration,
					replacement_term);
			},
		Term::UnaryOperation(unary_operation) =>
			replace_variable_in_term_with_term(&mut unary_operation.argument, variable_declaration,
				replacement_term),
		Term::Variable(variable) =>
			if *variable.declaration == *variable_declaration
			{
				*term = copy_term(replacement_term);
			},
		Term::Boolean(_)
		| Term::Integer(_)
		| Term::SpecialInteger(_)
		| Term::String(_) => (),
	}
}

pub(crate) fn replace_variable_in_formula_with_term(formula: &mut foliage::Formula,
	variable_declaration: &foliage::VariableDeclaration, replacement_term: &foliage::Term)
{
	use foliage::Formula;

	match formula
	{
		Formula::And(arguments)
		| Formula::IfAndOnlyIf(arguments)
		| Formula::Or(arguments) =>
			for argument in arguments
			{
				replace_variable_in_formula_with_term(argument, variable_declaration,
					replacement_term);
			},
		Formula::Compare(compare) =>
		{
			replace_variable_in_term_with_term(&mut compare.left, variable_declaration,
				replacement_term);
			replace_variable_in_term_with_term(&mut compare.right, variable_declaration,
				replacement_term);
		},
		Formula::Exists(quantified_formula)
		| Formula::ForAll(quantified_formula) =>
			replace_variable_in_formula_with_term(&mut quantified_formula.argument,
				variable_declaration, replacement_term),
		Formula::Implies(implies) =>
		{
			replace_variable_in_formula_with_term(&mut implies.antecedent, variable_declaration,
				replacement_term);
			replace_variable_in_formula_with_term(&mut implies.implication, variable_declaration,
				replacement_term);
		},
		Formula::Not(argument) =>
			replace_variable_in_formula_with_term(argument, variable_declaration, replacement_term),
		Formula::Predicate(predicate) =>
			for mut argument in &mut predicate.arguments
			{
				replace_variable_in_term_with_term(&mut argument, variable_declaration,
					replacement_term);
			},
		Formula::Boolean(_) => (),
	}
}

pub(crate) fn copy_term(term: &foliage::Term) -> foliage::Term
{
	use foliage::Term;

	match term
	{
		Term::BinaryOperation(binary_operation) =>
			Term::binary_operation(binary_operation.operator,
				Box::new(copy_term(&binary_operation.left)),
				Box::new(copy_term(&binary_operation.right))),
		Term::Boolean(value) => Term::boolean(*value),
		Term::Function(function) => Term::function(std::rc::Rc::clone(&function.declaration),
			function.arguments.iter().map(|argument| copy_term(argument)).collect()),
		Term::Integer(value) => Term::integer(*value),
		Term::SpecialInteger(value) => Term::special_integer(*value),
		Term::String(value) => Term::string(value.clone()),
		Term::UnaryOperation(unary_operation) => Term::unary_operation(unary_operation.operator,
			Box::new(copy_term(&unary_operation.argument))),
		Term::Variable(variable) => Term::variable(std::rc::Rc::clone(&variable.declaration)),
	}
}

fn copy_quantified_formula<D>(quantified_expression: &foliage::QuantifiedFormula, declarations: &D)
	-> foliage::QuantifiedFormula
where
	D: crate::traits::VariableDeclarationDomain + crate::traits::AssignVariableDeclarationDomain,
{
	let copy_parameters =
		quantified_expression.parameters.iter()
			.map(
				|parameter|
				{
					let copy_parameter = std::rc::Rc::new(
						foliage::VariableDeclaration::new(parameter.name.clone()));

					if let Some(domain) = declarations.variable_declaration_domain(parameter)
					{
						declarations.assign_variable_declaration_domain(&copy_parameter, domain);
					}

					copy_parameter
				})
			.collect();
	let copy_parameters = std::rc::Rc::new(copy_parameters);

	let mut copy_argument = copy_formula(&quantified_expression.argument, declarations);

	replace_variables_in_formula(&mut copy_argument, &quantified_expression.parameters,
		&copy_parameters);

	foliage::QuantifiedFormula::new(copy_parameters, Box::new(copy_argument))
}

pub(crate) fn copy_formula<D>(formula: &foliage::Formula, declarations: &D) -> foliage::Formula
where
	D: crate::traits::VariableDeclarationDomain + crate::traits::AssignVariableDeclarationDomain,
{
	use foliage::Formula;

	match formula
	{
		Formula::And(arguments) =>
			Formula::and(arguments.iter().map(
				|argument| copy_formula(argument, declarations)).collect()),
		Formula::Boolean(value) => Formula::boolean(*value),
		Formula::Compare(compare) =>
			Formula::compare(compare.operator, Box::new(copy_term(&compare.left)),
				Box::new(copy_term(&compare.right))),
		Formula::Exists(quantified_formula) =>
			Formula::Exists(copy_quantified_formula(quantified_formula, declarations)),
		Formula::ForAll(quantified_formula) =>
			Formula::ForAll(copy_quantified_formula(quantified_formula, declarations)),
		Formula::IfAndOnlyIf(arguments) =>
			Formula::if_and_only_if(
				arguments.iter().map(|argument| copy_formula(argument, declarations)).collect()),
		Formula::Implies(implies) =>
			Formula::implies(implies.direction,
				Box::new(copy_formula(&implies.antecedent, declarations)),
				Box::new(copy_formula(&implies.implication, declarations))),
		Formula::Not(argument) => Formula::not(Box::new(copy_formula(&argument, declarations))),
		Formula::Or(arguments) =>
			Formula::or(arguments.iter().map(
				|argument| copy_formula(argument, declarations)).collect()),
		Formula::Predicate(predicate) =>
			Formula::predicate(std::rc::Rc::clone(&predicate.declaration),
				predicate.arguments.iter().map(|argument| copy_term(argument)).collect()),
	}
}
