use super::ProofDirection;

#[derive(Eq, PartialEq)]
pub(crate) enum StatementKind
{
	Axiom,
	Assumption,
	CompletedDefinition(std::rc::Rc<foliage::PredicateDeclaration>),
	IntegrityConstraint,
	Lemma(ProofDirection),
	Assertion,
}

impl StatementKind
{
	pub fn display_capitalized(&self) -> StatementKindCapitalizedDisplay
	{
		StatementKindCapitalizedDisplay(self)
	}
}

impl std::fmt::Debug for StatementKind
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self
		{
			Self::Axiom => write!(formatter, "axiom"),
			Self::Assumption => write!(formatter, "assumption"),
			Self::CompletedDefinition(ref predicate_declaration) =>
				write!(formatter, "completed definition of {}", predicate_declaration),
			Self::IntegrityConstraint => write!(formatter, "integrity constraint"),
			Self::Lemma(_) => write!(formatter, "lemma"),
			Self::Assertion => write!(formatter, "assertion"),
		}
	}
}

impl std::fmt::Display for StatementKind
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

pub(crate) struct StatementKindCapitalizedDisplay<'s>(&'s StatementKind);

impl<'s> std::fmt::Debug for StatementKindCapitalizedDisplay<'s>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match self.0
		{
			StatementKind::Axiom => write!(formatter, "Axiom"),
			StatementKind::Assumption => write!(formatter, "Assumption"),
			StatementKind::CompletedDefinition(ref predicate_declaration) =>
				write!(formatter, "Completed definition of {}", predicate_declaration),
			StatementKind::IntegrityConstraint => write!(formatter, "Integrity constraint"),
			StatementKind::Lemma(_) => write!(formatter, "Lemma"),
			StatementKind::Assertion => write!(formatter, "Assertion"),
		}
	}
}

impl<'s> std::fmt::Display for StatementKindCapitalizedDisplay<'s>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

#[derive(Copy, Clone, Eq, PartialEq)]
pub(crate) enum ProofStatus
{
	AssumedProven,
	Proven,
	NotProven,
	Disproven,
	ToProveNow,
	ToProveLater,
	Ignored,
}

pub(crate) struct Statement
{
	pub kind: StatementKind,
	pub name: Option<String>,
	pub formula: foliage::Formula,
	pub proof_status: ProofStatus,
}

impl Statement
{
	pub fn new(kind: StatementKind, formula: foliage::Formula) -> Self
	{
		Self
		{
			kind,
			name: None,
			formula,
			proof_status: ProofStatus::ToProveLater,
		}
	}

	pub fn with_name(mut self, name: String) -> Self
	{
		self.name = Some(name);
		self
	}
}
