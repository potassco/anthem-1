pub(crate) fn is_term_arithmetic(term: &crate::Term) -> Result<bool, crate::Error>
{
	use crate::Term;

	match term
	{
		Term::Boolean(_)
		| Term::SpecialInteger(_)
		| Term::String(_)
			=> Ok(false),
		Term::Integer(_) => Ok(true),
		Term::Function(ref function) =>
		{
			if !function.arguments.is_empty()
			{
				return Err(
					crate::Error::new_unsupported_language_feature("functions with arguments"));
			}

			Ok(*function.declaration.domain.borrow() == crate::Domain::Integer)
		},
		Term::Variable(crate::Variable{ref declaration}) =>
			match declaration.domain()?
			{
				crate::Domain::Program => Ok(false),
				crate::Domain::Integer => Ok(true),
			},
		Term::BinaryOperation(crate::BinaryOperation{ref left, ref right, ..})
			=> Ok(is_term_arithmetic(left)? && is_term_arithmetic(right)?),
		Term::UnaryOperation(crate::UnaryOperation{ref argument, ..})
			=> is_term_arithmetic(argument),
	}
}
