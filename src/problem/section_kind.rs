#[derive(Clone, Copy, Eq, Ord, PartialEq, PartialOrd)]
pub enum SectionKind
{
	CompletedDefinitions,
	IntegrityConstraints,
	Axioms,
	Assumptions,
	Lemmas,
	Assertions,
}

impl SectionKind
{
	pub fn display_capitalized(&self) -> SectionKindCapitalizedDisplay
	{
		SectionKindCapitalizedDisplay(*self)
	}
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

pub struct SectionKindCapitalizedDisplay(SectionKind);

impl std::fmt::Debug for SectionKindCapitalizedDisplay
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self.0
		{
			SectionKind::CompletedDefinitions => write!(formatter, "Completed definition"),
			SectionKind::IntegrityConstraints => write!(formatter, "Integrity constraint"),
			SectionKind::Axioms => write!(formatter, "Axiom"),
			SectionKind::Assumptions => write!(formatter, "Assumption"),
			SectionKind::Lemmas => write!(formatter, "Lemma"),
			SectionKind::Assertions => write!(formatter, "Assertion"),
		}
	}
}

impl std::fmt::Display for SectionKindCapitalizedDisplay
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}
