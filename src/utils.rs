mod arithmetic_terms;
mod closures;

pub(crate) use arithmetic_terms::*;
pub(crate) use closures::*;

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

pub type InputConstantDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::FunctionDeclaration>, Domain>;
