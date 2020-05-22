// TODO: remove if possible
#[derive(Clone, Copy, Eq, Ord, PartialEq, PartialOrd)]
pub enum SectionKind
{
	Axioms,
	Assumptions,
	Lemmas,
	CompletedDefinitions,
	IntegrityConstraints,
	Specs,
}

impl std::fmt::Debug for SectionKind
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self
		{
			Self::CompletedDefinitions => write!(formatter, "completed definition"),
			Self::IntegrityConstraints => write!(formatter, "integrity constraint"),
			Self::Axioms => write!(formatter, "axiom"),
			Self::Assumptions => write!(formatter, "assumption"),
			Self::Lemmas => write!(formatter, "lemma"),
			Self::Specs => write!(formatter, "spec"),
		}
	}
}

impl std::fmt::Display for SectionKind
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}
