mod arithmetic_terms;

pub(crate) use arithmetic_terms::*;

#[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd)]
pub(crate) enum OperatorNotation
{
	Prefix,
	Infix,
}

#[derive(Clone, Copy, Eq, Ord, PartialEq, PartialOrd)]
pub enum Domain
{
	Program,
	Integer,
}

impl std::fmt::Debug for Domain
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self
		{
			Domain::Program => write!(formatter, "program"),
			Domain::Integer => write!(formatter, "integer"),
		}
	}
}

impl std::fmt::Display for Domain
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

#[derive(Clone, Copy, Eq, Hash, PartialEq)]
pub enum ProofDirection
{
	Forward,
	Backward,
	Both,
}

impl std::fmt::Debug for ProofDirection
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self
		{
			ProofDirection::Forward => write!(formatter, "forward"),
			ProofDirection::Backward => write!(formatter, "backward"),
			ProofDirection::Both => write!(formatter, "both"),
		}
	}
}

impl std::fmt::Display for ProofDirection
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

pub struct InvalidProofDirectionError;

impl std::fmt::Debug for InvalidProofDirectionError
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "invalid proof direction")
	}
}

impl std::fmt::Display for InvalidProofDirectionError
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

impl std::str::FromStr for ProofDirection
{
	type Err = InvalidProofDirectionError;

	fn from_str(s: &str) -> Result<Self, Self::Err>
	{
		match s
		{
			"forward" => Ok(ProofDirection::Forward),
			"backward" => Ok(ProofDirection::Backward),
			"both" => Ok(ProofDirection::Both),
			_ => Err(InvalidProofDirectionError),
		}
	}
}

pub(crate) struct ScopedFormula
{
	pub free_variable_declarations: std::rc::Rc<foliage::VariableDeclarations>,
	pub formula: foliage::Formula,
}

pub(crate) fn existential_closure(scoped_formula: crate::ScopedFormula) -> foliage::Formula
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => foliage::Formula::exists(scoped_formula.free_variable_declarations,
			Box::new(scoped_formula.formula)),
	}
}

pub(crate) fn universal_closure(scoped_formula: crate::ScopedFormula) -> foliage::Formula
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => foliage::Formula::for_all(scoped_formula.free_variable_declarations,
			Box::new(scoped_formula.formula)),
	}
}

/*pub fn parse_predicate_declaration(input: &str)
	-> Result<std::rc::Rc<foliage::PredicateDeclaration>, crate::Error>
{
	let mut parts = input.split("/");

	let name = parts.next().ok_or(crate::Error::new_parse_predicate_declaration())?;

	use std::str::FromStr;

	let arity = match parts.next()
	{
		Some(arity)
			=> usize::from_str(arity).map_err(|_| crate::Error::new_parse_predicate_declaration())?,
		None => 0,
	};

	if parts.next().is_some()
	{
		return Err(crate::Error::new_parse_predicate_declaration());
	}

	Ok(std::rc::Rc::new(foliage::PredicateDeclaration
	{
		name: name.to_string(),
		arity,
	}))
}*/

pub type InputConstantDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::FunctionDeclaration>, Domain>;

/*pub fn parse_constant_declaration(input: &str)
	-> Result<(std::rc::Rc<foliage::FunctionDeclaration>, crate::Domain), crate::Error>
{
	let mut parts = input.split(":");

	let name = parts.next().ok_or(crate::Error::new_parse_constant_declaration())?.trim();

	let domain = match parts.next()
	{
		None => crate::Domain::Program,
		Some(value) => match value.trim()
		{
			"program" => crate::Domain::Program,
			"integer" => crate::Domain::Integer,
			_ => return Err(crate::Error::new_parse_constant_declaration()),
		},
	};

	if parts.next().is_some()
	{
		return Err(crate::Error::new_parse_constant_declaration());
	}

	let constant_declaration = std::rc::Rc::new(foliage::FunctionDeclaration
	{
		name: name.to_string(),
		arity: 0,
	});

	Ok((constant_declaration, domain))
}*/
