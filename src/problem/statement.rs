use super::ProofDirection;

#[derive(Eq, PartialEq)]
pub(crate) enum StatementKind
{
	Axiom,
	Assumption,
	CompletedDefinition(std::rc::Rc<crate::PredicateDeclaration>),
	IntegrityConstraint,
	Lemma(ProofDirection),
	Spec,
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
				write!(formatter, "completed definition of {}", predicate_declaration.declaration),
			Self::IntegrityConstraint => write!(formatter, "integrity constraint"),
			Self::Lemma(_) => write!(formatter, "lemma"),
			Self::Spec => write!(formatter, "spec"),
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
	pub formula: crate::Formula,
	pub proof_status: ProofStatus,
}

impl Statement
{
	pub fn new(kind: StatementKind, formula: crate::Formula) -> Self
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
