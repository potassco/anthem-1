mod arithmetic_terms;

pub(crate) use arithmetic_terms::*;

#[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd)]
pub(crate) enum OperatorNotation
{
	Prefix,
	Infix,
}

#[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd)]
pub(crate) enum Domain
{
	Program,
	Integer,
}

pub(crate) struct ScopedFormula
{
	pub free_variable_declarations: std::rc::Rc<foliage::VariableDeclarations>,
	pub formula: Box<foliage::Formula>,
}

pub fn parse_predicate_declaration(input: &str)
	-> Result<std::rc::Rc<foliage::PredicateDeclaration>, crate::Error>
{
	let mut parts = input.split("/");

	let name = match parts.next()
	{
		Some(name) => name.to_string(),
		None => return Err(crate::Error::new_parse_predicate_declaration()),
	};

	use std::str::FromStr;

	let arity = match parts.next()
	{
		Some(arity)
			=> usize::from_str(arity).map_err(|_| crate::Error::new_parse_predicate_declaration())?,
		None => return Err(crate::Error::new_parse_predicate_declaration()),
	};

	if parts.next().is_some()
	{
		return Err(crate::Error::new_parse_predicate_declaration());
	}

	Ok(std::rc::Rc::new(foliage::PredicateDeclaration
	{
		name,
		arity,
	}))
}
