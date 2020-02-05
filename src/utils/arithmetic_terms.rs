pub(crate) fn is_term_arithmetic<C>(term: &foliage::Term, context: &C) -> Result<bool, crate::Error>
where
	C: crate::traits::InputConstantDeclarationDomain
		+ crate::traits::VariableDeclarationDomain
{
	match term
	{
		foliage::Term::Boolean(_)
		| foliage::Term::SpecialInteger(_)
		| foliage::Term::String(_)
			=> Ok(false),
		foliage::Term::Integer(_) => Ok(true),
		foliage::Term::Function(ref function) =>
		{
			if !function.arguments.is_empty()
			{
				return Err(
					crate::Error::new_unsupported_language_feature("functions with arguments"));
			}

			let domain = context.input_constant_declaration_domain(&function.declaration);

			Ok(domain == crate::Domain::Integer)
		},
		foliage::Term::Variable(foliage::Variable{ref declaration}) =>
			match context.variable_declaration_domain(declaration)
			{
				Some(crate::Domain::Program) => Ok(false),
				Some(crate::Domain::Integer) => Ok(true),
				None => Err(crate::Error::new_logic("unspecified variable declaration domain")),
			},
		foliage::Term::BinaryOperation(foliage::BinaryOperation{ref left, ref right, ..})
			=> Ok(is_term_arithmetic(left, context)? && is_term_arithmetic(right, context)?),
		foliage::Term::UnaryOperation(foliage::UnaryOperation{ref argument, ..})
			=> is_term_arithmetic(argument, context),
	}
}
