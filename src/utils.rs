mod arithmetic_terms;
mod autoname_variables;
mod closures;
mod collect_predicate_declarations;
mod copy_formula;
mod formula_contains_predicate;
mod variables_in_terms;

pub(crate) use autoname_variables::*;
pub(crate) use arithmetic_terms::*;
pub(crate) use closures::*;
pub(crate) use collect_predicate_declarations::*;
pub(crate) use copy_formula::*;
pub(crate) use formula_contains_predicate::*;
pub(crate) use variables_in_terms::*;

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
