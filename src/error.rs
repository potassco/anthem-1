use foliage::flavor::VariableDeclaration as _;

pub type Source = Box<dyn std::error::Error>;

pub enum Kind
{
	Logic(&'static str),
	UnsupportedLanguageFeature(&'static str),
	NotYetImplemented(&'static str),
	DecodeIdentifier,
	Translate,
	ReadFile(std::path::PathBuf),
	ExpectedStatement,
	ExpectedColon,
	UnknownStatement(String),
	UnmatchedParenthesis,
	MissingStatementTerminator,
	ParseFormula,
	ExpectedIdentifier,
	ExpectedPredicateSpecifier,
	ParsePredicateDeclaration,
	//ParseConstantDeclaration,
	UnknownProofDirection(String),
	UnknownDomainIdentifier(String),
	UnknownColorChoice(String),
	VariableNameNotAllowed(String),
	FormulaNotClosed(std::rc::Rc<crate::VariableDeclarations>),
	NoCompletedDefinitionFound(std::rc::Rc<crate::PredicateDeclaration>),
	PrivatePredicateCycle(std::rc::Rc<crate::PredicateDeclaration>),
	PrivatePredicateInSpecification(std::rc::Rc<crate::PredicateDeclaration>),
	PrivatePredicateDependingOnPublicPredicate(std::rc::Rc<crate::PredicateDeclaration>,
		std::rc::Rc<crate::PredicateDeclaration>),
	RunVampire,
	// TODO: rename to something Vampire-specific
	ProveProgram(Option<i32>, String, String),
	ParseVampireOutput(String, String),
	IO,
}

pub struct Error
{
	pub kind: Kind,
	pub source: Option<Source>,
}

impl Error
{
	pub(crate) fn new(kind: Kind) -> Self
	{
		Self
		{
			kind,
			source: None,
		}
	}

	pub(crate) fn with<S: Into<Source>>(mut self, source: S) -> Self
	{
		self.source = Some(source.into());
		self
	}

	pub(crate) fn new_logic(description: &'static str) -> Self
	{
		Self::new(Kind::Logic(description))
	}

	pub(crate) fn new_unsupported_language_feature(description: &'static str) -> Self
	{
		Self::new(Kind::UnsupportedLanguageFeature(description))
	}

	pub(crate) fn new_not_yet_implemented(description: &'static str) -> Self
	{
		Self::new(Kind::NotYetImplemented(description))
	}

	pub(crate) fn new_decode_identifier<S: Into<Source>>(source: S) -> Self
	{
		Self::new(Kind::DecodeIdentifier).with(source)
	}

	pub(crate) fn new_translate<S: Into<Source>>(source: S) -> Self
	{
		Self::new(Kind::Translate).with(source)
	}

	pub(crate) fn new_read_file<S: Into<Source>>(path: std::path::PathBuf, source: S) -> Self
	{
		Self::new(Kind::ReadFile(path)).with(source)
	}

	pub(crate) fn new_expected_statement() -> Self
	{
		Self::new(Kind::ExpectedStatement)
	}

	pub(crate) fn new_expected_colon() -> Self
	{
		Self::new(Kind::ExpectedColon)
	}

	pub(crate) fn new_unknown_statement(statement_name: String) -> Self
	{
		Self::new(Kind::UnknownStatement(statement_name))
	}

	pub(crate) fn new_unmatched_parenthesis() -> Self
	{
		Self::new(Kind::UnmatchedParenthesis)
	}

	pub(crate) fn new_missing_statement_terminator() -> Self
	{
		Self::new(Kind::MissingStatementTerminator)
	}

	pub(crate) fn new_parse_formula<S: Into<Source>>(source: S) -> Self
	{
		Self::new(Kind::ParseFormula).with(source)
	}

	pub(crate) fn new_expected_identifier() -> Self
	{
		Self::new(Kind::ExpectedIdentifier)
	}

	pub(crate) fn new_expected_predicate_specifier() -> Self
	{
		Self::new(Kind::ExpectedPredicateSpecifier)
	}

	pub(crate) fn new_parse_predicate_declaration() -> Self
	{
		Self::new(Kind::ParsePredicateDeclaration)
	}

	pub(crate) fn new_unknown_proof_direction(proof_direction: String) -> Self
	{
		Self::new(Kind::UnknownProofDirection(proof_direction))
	}

	pub(crate) fn new_unknown_domain_identifier(domain_identifier: String) -> Self
	{
		Self::new(Kind::UnknownDomainIdentifier(domain_identifier))
	}

	pub(crate) fn new_unknown_color_choice(color_choice: String) -> Self
	{
		Self::new(Kind::UnknownColorChoice(color_choice))
	}

	pub(crate) fn new_variable_name_not_allowed(variable_name: String) -> Self
	{
		Self::new(Kind::VariableNameNotAllowed(variable_name))
	}

	pub(crate) fn new_formula_not_closed(free_variables: std::rc::Rc<crate::VariableDeclarations>)
		-> Self
	{
		Self::new(Kind::FormulaNotClosed(free_variables))
	}

	pub(crate) fn new_no_completed_definition_found(
		predicate_declaration: std::rc::Rc<crate::PredicateDeclaration>)
		-> Self
	{
		Self::new(Kind::NoCompletedDefinitionFound(predicate_declaration))
	}

	pub(crate) fn new_private_predicate_cycle(
		predicate_declaration: std::rc::Rc<crate::PredicateDeclaration>)
		-> Self
	{
		Self::new(Kind::PrivatePredicateCycle(predicate_declaration))
	}

	pub(crate) fn new_private_predicate_in_specification(
		predicate_declaration: std::rc::Rc<crate::PredicateDeclaration>)
		-> Self
	{
		Self::new(Kind::PrivatePredicateInSpecification(predicate_declaration))
	}

	pub(crate) fn new_private_predicate_depending_on_public_predicate(
		private_predicate_declaration: std::rc::Rc<crate::PredicateDeclaration>,
		public_predicate_declaration: std::rc::Rc<crate::PredicateDeclaration>)
		-> Self
	{
		Self::new(Kind::PrivatePredicateDependingOnPublicPredicate(private_predicate_declaration,
			public_predicate_declaration))
	}

	pub(crate) fn new_run_vampire<S: Into<Source>>(source: S) -> Self
	{
		Self::new(Kind::RunVampire).with(source)
	}

	pub(crate) fn new_prove_program(exit_code: Option<i32>, stdout: String, stderr: String) -> Self
	{
		Self::new(Kind::ProveProgram(exit_code, stdout, stderr))
	}

	pub(crate) fn new_parse_vampire_output(stdout: String, stderr: String) -> Self
	{
		Self::new(Kind::ParseVampireOutput(stdout, stderr))
	}

	pub(crate) fn new_io<S: Into<Source>>(source: S) -> Self
	{
		Self::new(Kind::IO).with(source)
	}
}

impl std::fmt::Debug for Error
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		match &self.kind
		{
			Kind::Logic(ref description) => write!(formatter,
				"logic error, please report to bug tracker ({})", description),
			Kind::UnsupportedLanguageFeature(ref description) => write!(formatter,
				"language feature not yet supported ({})", description),
			Kind::NotYetImplemented(ref description) => write!(formatter,
				"not yet implemented ({})", description),
			Kind::DecodeIdentifier => write!(formatter, "could not decode identifier"),
			Kind::Translate => write!(formatter, "could not translate input program"),
			Kind::ReadFile(path) => write!(formatter, "could not read file “{}”", path.display()),
			Kind::ExpectedStatement => write!(formatter,
				"expected statement (axiom, assert, assume, input, lemma)"),
			Kind::ExpectedColon => write!(formatter, "expected ‘:’"),
			Kind::UnknownStatement(ref statement_name) => write!(formatter,
				"unknown statement “{}” (allowed: axiom, assert, assume, input, lemma)",
				statement_name),
			Kind::UnmatchedParenthesis => write!(formatter, "unmatched parenthesis"),
			Kind::ParseFormula => write!(formatter, "could not parse formula"),
			Kind::ExpectedIdentifier => write!(formatter, "expected constant or predicate name"),
			Kind::ExpectedPredicateSpecifier =>
				write!(formatter, "expected predicate specifier (examples: p/0, q/2)"),
			Kind::ParsePredicateDeclaration => write!(formatter,
				"could not parse predicate declaration"),
			// TODO: rename to ExpectedStatementTerminator
			Kind::MissingStatementTerminator => write!(formatter,
				"statement not terminated with ‘.’ character"),
			Kind::UnknownProofDirection(ref proof_direction) => write!(formatter,
				"unknown proof direction “{}” (allowed: integer, program)", proof_direction),
			Kind::UnknownDomainIdentifier(ref domain_identifier) => write!(formatter,
				"unknown domain identifier “{}” (allowed: int, program)", domain_identifier),
			Kind::UnknownColorChoice(ref color_choice) => write!(formatter,
				"unknown color choice “{}” (allowed: auto, always, never)", color_choice),
			Kind::VariableNameNotAllowed(ref variable_name) => write!(formatter,
				"variable name “{}” not allowed (program variables must start with X, Y, or Z and integer variables with I, J, K, L, M, or N)",
				variable_name),
			Kind::FormulaNotClosed(free_variable_declarations) =>
			{
				write!(formatter, "formula may not contain free variables (free variables in this formula: ")?;

				let mut separator = "";

				for free_variable_declaration in &**free_variable_declarations
				{
					write!(formatter, "{}", separator)?;
					free_variable_declaration.display_name(formatter)?;
					separator = ", ";
				}

				write!(formatter, ")")
			},
			Kind::NoCompletedDefinitionFound(ref predicate_declaration) =>
				write!(formatter, "no completed definition found for {}", predicate_declaration.declaration),
			Kind::PrivatePredicateCycle(ref predicate_declaration) =>
				write!(formatter,
					"program is not supertight (private predicate {} transitively depends on itself)",
					predicate_declaration.declaration),
			Kind::PrivatePredicateInSpecification(ref predicate_declaration) =>
				write!(formatter,
					"private predicate {} should not occur in specification (consider declaring it an input or output predicate)",
					predicate_declaration.declaration),
			Kind::PrivatePredicateDependingOnPublicPredicate(ref private_predicate_declaration,
				ref public_predicate_declaration) =>
				write!(formatter,
					"private predicate {} transitively depends on public predicate {}",
					private_predicate_declaration.declaration,
					public_predicate_declaration.declaration),
			Kind::RunVampire => write!(formatter, "could not run Vampire"),
			Kind::ProveProgram(exit_code, ref stdout, ref stderr) =>
			{
				let exit_code_output = match exit_code
				{
					None => "no exit code".to_string(),
					Some(exit_code) => format!("exit code: {}", exit_code),
				};

				write!(formatter,
					"error proving program ({})\n\
					==== stdout ===========================================================\n\
					{}\
					==== stderr ===========================================================\n\
					{}", exit_code_output, stdout, stderr)
			},
			Kind::ParseVampireOutput(ref stdout, ref stderr) => write!(formatter,
				"could not parse Vampire output\n\
				==== stdout ===========================================================\n\
				{}\
				==== stderr ===========================================================\n\
				{}", stdout, stderr),
			Kind::IO => write!(formatter, "input/output error"),
		}?;

		if let Some(source) = &self.source
		{
			write!(formatter, "\nerror source: {}", source)?;
		}

		Ok(())
	}
}

impl std::fmt::Display for Error
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		write!(formatter, "{:?}", self)
	}
}

impl std::error::Error for Error
{
	fn source(&self) -> Option<&(dyn std::error::Error + 'static)>
	{
		match &self.source
		{
			Some(source) => Some(source.as_ref()),
			None => None,
		}
	}
}

impl From<std::io::Error> for Error
{
	fn from(error: std::io::Error) -> Self
	{
		Self::new_io(error)
	}
}
