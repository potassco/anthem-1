// TODO: remove if possible
#[derive(Clone, Copy, Eq, Ord, PartialEq, PartialOrd)]
pub enum SectionKind
{
	Axioms,
	Assumptions,
	Lemmas,
	CompletedDefinitions,
	IntegrityConstraints,
	Assertions,
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
			Self::Assertions => write!(formatter, "assertion"),
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
