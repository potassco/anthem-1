#[derive(Copy, Clone, Eq, PartialEq)]
pub enum StatementKind
{
	Axiom,
	Program,
	Assumption,
	Lemma(crate::ProofDirection),
	Assertion,
}

#[derive(Copy, Clone, Eq, PartialEq)]
enum ProofStatus
{
	AssumedProven,
	ToProveNow,
	ToProveLater,
	Ignored,
}

#[derive(Copy, Clone, Eq, PartialEq)]
pub enum ProofResult
{
	Proven,
	NotProven,
	Disproven,
}

pub struct Statement
{
	kind: StatementKind,
	name: Option<String>,
	description: Option<String>,
	formula: foliage::Formula,
	proof_status: ProofStatus,
}

type VariableDeclarationIDs
	= std::collections::BTreeMap::<std::rc::Rc<foliage::VariableDeclaration>, usize>;

struct FormatContext<'a, 'b>
{
	pub program_variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
	pub integer_variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
	pub input_constant_declaration_domains: &'a crate::InputConstantDeclarationDomains,
	pub variable_declaration_domains: &'b VariableDeclarationDomains,
}

impl Statement
{
	pub fn new(kind: StatementKind, formula: foliage::Formula) -> Self
	{
		Self
		{
			kind,
			name: None,
			description: None,
			formula,
			proof_status: ProofStatus::ToProveLater,
		}
	}

	pub fn with_name(mut self, name: String) -> Self
	{
		self.name = Some(name);
		self
	}

	pub fn with_description(mut self, description: String) -> Self
	{
		self.description = Some(description);
		self
	}
}

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

type VariableDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::VariableDeclaration>, crate::Domain>;

pub struct Problem
{
	function_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,

	statements: std::cell::RefCell<std::collections::BTreeMap<SectionKind, Vec<Statement>>>,

	pub input_constant_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub input_constant_declaration_domains:
		std::cell::RefCell<crate::InputConstantDeclarationDomains>,
	pub input_predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,
	// TODO: clean up as variable declarations are dropped
	variable_declaration_domains: std::cell::RefCell<VariableDeclarationDomains>,
}

impl Problem
{
	pub fn new() -> Self
	{
		Self
		{
			function_declarations: std::cell::RefCell::new(foliage::FunctionDeclarations::new()),
			predicate_declarations: std::cell::RefCell::new(foliage::PredicateDeclarations::new()),

			statements: std::cell::RefCell::new(std::collections::BTreeMap::new()),

			input_constant_declarations:
				std::cell::RefCell::new(foliage::FunctionDeclarations::new()),
			input_constant_declaration_domains:
				std::cell::RefCell::new(crate::InputConstantDeclarationDomains::new()),
			input_predicate_declarations:
				std::cell::RefCell::new(foliage::PredicateDeclarations::new()),
			variable_declaration_domains:
				std::cell::RefCell::new(VariableDeclarationDomains::new()),
		}
	}

	pub fn add_statement(&self, section_kind: SectionKind, statement: Statement)
	{
		let mut statements = self.statements.borrow_mut();
		let section = statements.entry(section_kind).or_insert(vec![]);

		section.push(statement);
	}

	pub fn prove(&self, proof_direction: crate::ProofDirection) -> Result<(), crate::Error>
	{
		// TODO: refactor
		let format_context = FormatContext
		{
			program_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			integer_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			input_constant_declaration_domains: &self.input_constant_declaration_domains.borrow(),
			variable_declaration_domains: &self.variable_declaration_domains.borrow(),
		};

		if proof_direction == crate::ProofDirection::Forward
			|| proof_direction == crate::ProofDirection::Both
		{
			println!("performing forward proof");

			let mut statements = self.statements.borrow_mut();

			// Initially reset all proof statuses
			for (section_kind, statements) in statements.iter_mut()
			{
				for statement in statements.iter_mut()
				{
					match statement.kind
					{
						StatementKind::Axiom
						| StatementKind::Assumption
						| StatementKind::Program =>
						{
							// TODO: avoid code duplication
							let statement_type_output = match section_kind
							{
								SectionKind::CompletedDefinitions => "completed definition",
								SectionKind::IntegrityConstraints => "integrity constraint",
								SectionKind::Axioms => "axiom",
								SectionKind::Assumptions => "assumption",
								SectionKind::Lemmas => "lemma",
								SectionKind::Assertions => "assertion",
							};

							// TODO: refactor
							match statement.kind
							{
								StatementKind::Program => println!("  - {}: {}",
									statement_type_output,
									foliage::format::display_formula(&statement.formula,
										&format_context)),
								_ => println!("  - {}: {}", statement_type_output,
									statement.formula),
							}

							statement.proof_status = ProofStatus::AssumedProven;
						},
						StatementKind::Lemma(crate::ProofDirection::Backward) =>
							statement.proof_status = ProofStatus::Ignored,
						_ => statement.proof_status = ProofStatus::ToProveLater,
					}
				}
			}

			drop(statements);

			self.prove_unproven_statements()?;

			println!("finished forward proof");
		}

		if proof_direction == crate::ProofDirection::Backward
			|| proof_direction == crate::ProofDirection::Both
		{
			println!("performing backward proof");

			let mut statements = self.statements.borrow_mut();

			// Initially reset all proof statuses
			for (section_kind, statements) in statements.iter_mut()
			{
				for statement in statements.iter_mut()
				{
					match statement.kind
					{
						StatementKind::Axiom
						| StatementKind::Assumption
						| StatementKind::Assertion =>
						{
							// TODO: avoid code duplication
							let statement_type_output = match section_kind
							{
								SectionKind::CompletedDefinitions => "completed definition",
								SectionKind::IntegrityConstraints => "integrity constraint",
								SectionKind::Axioms => "axiom",
								SectionKind::Assumptions => "assumption",
								SectionKind::Lemmas => "lemma",
								SectionKind::Assertions => "assertion",
							};

							// TODO: refactor
							match statement.kind
							{
								StatementKind::Program => println!("  - {}: {}",
									statement_type_output,
									foliage::format::display_formula(&statement.formula,
										&format_context)),
								_ => println!("  - {}: {}", statement_type_output,
									statement.formula),
							}

							statement.proof_status = ProofStatus::AssumedProven;
						},
						StatementKind::Lemma(crate::ProofDirection::Forward) =>
							statement.proof_status = ProofStatus::Ignored,
						_ => statement.proof_status = ProofStatus::ToProveLater,
					}
				}
			}

			drop(statements);

			self.prove_unproven_statements()?;

			println!("finished backward proof");
		}

		Ok(())
	}

	fn next_unproven_statement_do_mut<F, G>(&self, mut functor: F) -> Option<G>
	where
		F: FnMut(SectionKind, &mut Statement) -> G,
	{
		for section in self.statements.borrow_mut().iter_mut()
		{
			for statement in section.1.iter_mut()
			{
				if statement.proof_status == ProofStatus::ToProveNow
					|| statement.proof_status == ProofStatus::ToProveLater
				{
					return Some(functor(*section.0, statement));
				}
			}
		}

		None
	}

	fn prove_unproven_statements(&self) -> Result<(), crate::Error>
	{
		let format_context = FormatContext
		{
			program_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			integer_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			input_constant_declaration_domains: &self.input_constant_declaration_domains.borrow(),
			variable_declaration_domains: &self.variable_declaration_domains.borrow(),
		};

		loop
		{
			match self.next_unproven_statement_do_mut(
				|section_kind, statement|
				{
					statement.proof_status = ProofStatus::ToProveNow;

					let statement_type_output = match section_kind
					{
						SectionKind::CompletedDefinitions => "completed definition",
						SectionKind::IntegrityConstraints => "integrity constraint",
						SectionKind::Axioms => "axiom",
						SectionKind::Assumptions => "assumption",
						SectionKind::Lemmas => "lemma",
						SectionKind::Assertions => "assertion",
					};

					match statement.kind
					{
						StatementKind::Program => println!("  - verifying {}: {}",
							statement_type_output,
							foliage::format::display_formula(&statement.formula, &format_context)),
						_ => println!("  - verifying {}: {}", statement_type_output,
							statement.formula),
					}
				})
			{
				Some(_) => (),
				None => break,
			}

			let mut tptp_problem_to_prove_next_statement = "".to_string();

			self.write_tptp_problem_to_prove_next_statement(
				&mut tptp_problem_to_prove_next_statement)
				.map_err(|error| crate::Error::new_write_tptp_program(error))?;

			log::trace!("TPTP program:\n{}", &tptp_problem_to_prove_next_statement);

			// TODO: make configurable again
			let (proof_result, proof_time_seconds) =
				run_vampire(&tptp_problem_to_prove_next_statement,
					Some(&["--mode", "casc", "--cores", "8", "--time_limit", "300"]))?;

			match proof_result
			{
				ProofResult::NotProven =>
				{
					println!("    → could not prove statement");

					return Ok(());
				},
				ProofResult::Disproven =>
				{
					println!("    → statement disproven");

					return Ok(());
				},
				_ => (),
			}

			match self.next_unproven_statement_do_mut(
				|_, statement| statement.proof_status = ProofStatus::AssumedProven)
			{
				Some(_) => (),
				None => unreachable!("could not set the statement to proven"),
			}

			match proof_time_seconds
			{
				None => println!("    → statement proven"),
				Some(proof_time_seconds) =>
					println!("    → statement proven in {} seconds", proof_time_seconds),
			}
		}

		Ok(())
	}

	fn display(&self, output_format: crate::output::Format)
	{
		let format_context = FormatContext
		{
			program_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			integer_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			input_constant_declaration_domains: &self.input_constant_declaration_domains.borrow(),
			variable_declaration_domains: &self.variable_declaration_domains.borrow(),
		};

		let print_title = |title, section_separator|
		{
			print!("{}{}", section_separator, "%".repeat(72));
			print!("\n% {}", title);
			println!("\n{}", "%".repeat(72));
		};

		let print_formula = |formula: &foliage::Formula|
		{
			match output_format
			{
				crate::output::Format::HumanReadable => print!("{}",
					foliage::format::display_formula(formula, &format_context)),
				crate::output::Format::TPTP => print!("{}",
					crate::output::tptp::display_formula(formula, &format_context)),
			}
		};

		let mut section_separator = "";

		if output_format == crate::output::Format::TPTP
		{
			print_title("anthem types", section_separator);
			section_separator = "\n";

			let tptp_preamble_anthem_types
				= include_str!("output/tptp/preamble_types.tptp").trim_end();
			println!("{}", tptp_preamble_anthem_types);

			print_title("anthem axioms", section_separator);

			let tptp_preamble_anthem_types
				= include_str!("output/tptp/preamble_axioms.tptp").trim_end();
			println!("{}", tptp_preamble_anthem_types);

			if !self.predicate_declarations.borrow().is_empty()
				|| !self.function_declarations.borrow().is_empty()
			{
				print_title("types", section_separator);

				if !self.predicate_declarations.borrow().is_empty()
				{
					println!("% predicate types")
				}

				for predicate_declaration in self.predicate_declarations.borrow().iter()
				{
					println!("tff(type, type, {}).",
						crate::output::tptp::display_predicate_declaration(predicate_declaration));
				}

				if !self.function_declarations.borrow().is_empty()
				{
					println!("% function types")
				}

				for function_declaration in self.function_declarations.borrow().iter()
				{
					println!("tff(type, type, {}).",
						crate::output::tptp::display_function_declaration(function_declaration,
							&format_context));
				}
			}

			let function_declarations = self.function_declarations.borrow();
			let symbolic_constants = function_declarations.iter().filter(
				|x| !self.input_constant_declaration_domains.borrow().contains_key(*x));

			let mut last_symbolic_constant: Option<std::rc::Rc<foliage::FunctionDeclaration>> =
				None;

			for (i, symbolic_constant) in symbolic_constants.enumerate()
			{
				// Order axioms are only necessary with two or more symbolic constants
				if i == 1
				{
					println!("% axioms for order of symbolic constants")
				}

				if symbolic_constant.arity > 0
				{
					// TODO: refactor
					panic!("unexpected n-ary function declaration");
				}

				if let Some(last_symbolic_constant) = last_symbolic_constant
				{
					println!("tff(symbolic_constant_order, axiom, p__less__({}, {})).",
						last_symbolic_constant.name, symbolic_constant.name);
				}

				last_symbolic_constant = Some(std::rc::Rc::clone(symbolic_constant));
			}
		}

		for (section_kind, statements) in self.statements.borrow().iter()
		{
			if statements.is_empty()
			{
				continue;
			}

			let title = match section_kind
			{
				SectionKind::CompletedDefinitions => "completed definitions",
				SectionKind::IntegrityConstraints => "integrity constraints",
				SectionKind::Axioms => "axioms",
				SectionKind::Assumptions => "assumptions",
				SectionKind::Lemmas => "lemmas",
				SectionKind::Assertions => "assertions",
			};

			print_title(title, section_separator);
			section_separator = "\n";

			let mut i = 0;

			for statement in statements.iter()
			{
				if let Some(ref description) = statement.description
				{
					println!("% {}", description);
				}

				let name = match &statement.name
				{
					// TODO: refactor
					Some(name) => name.clone(),
					None =>
					{
						i += 1;
						format!("statement_{}", i)
					},
				};

				if output_format == crate::output::Format::TPTP
				{
					// TODO: add proper statement type
					print!("tff({}, axiom, ", name);
				}

				print_formula(&statement.formula);

				if output_format == crate::output::Format::TPTP
				{
					print!(").");
				}

				println!("");
			}
		}
	}

	fn write_tptp_problem_to_prove_next_statement(&self, formatter: &mut String) -> std::fmt::Result
	{
		use std::fmt::Write as _;

		let format_context = FormatContext
		{
			program_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			integer_variable_declaration_ids:
				std::cell::RefCell::new(VariableDeclarationIDs::new()),
			input_constant_declaration_domains: &self.input_constant_declaration_domains.borrow(),
			variable_declaration_domains: &self.variable_declaration_domains.borrow(),
		};

		let write_title = |formatter: &mut String, title, section_separator| -> std::fmt::Result
		{
			write!(formatter, "{}{}", section_separator, "%".repeat(72))?;
			write!(formatter, "\n% {}", title)?;
			writeln!(formatter, "\n{}", "%".repeat(72))
		};

		let mut section_separator = "";

		write_title(formatter, "anthem types", section_separator)?;
		section_separator = "\n";

		let tptp_preamble_anthem_types
			= include_str!("output/tptp/preamble_types.tptp").trim_end();
		writeln!(formatter, "{}", tptp_preamble_anthem_types)?;

		write_title(formatter, "anthem axioms", section_separator)?;

		let tptp_preamble_anthem_types
			= include_str!("output/tptp/preamble_axioms.tptp").trim_end();
		writeln!(formatter, "{}", tptp_preamble_anthem_types)?;

		if !self.predicate_declarations.borrow().is_empty()
			|| !self.function_declarations.borrow().is_empty()
		{
			write_title(formatter, "types", section_separator)?;

			if !self.predicate_declarations.borrow().is_empty()
			{
				writeln!(formatter, "% predicate types")?;
			}

			for predicate_declaration in self.predicate_declarations.borrow().iter()
			{
				writeln!(formatter, "tff(type, type, {}).",
					crate::output::tptp::display_predicate_declaration(predicate_declaration))?;
			}

			if !self.function_declarations.borrow().is_empty()
			{
				writeln!(formatter, "% function types")?;
			}

			for function_declaration in self.function_declarations.borrow().iter()
			{
				writeln!(formatter, "tff(type, type, {}).",
					crate::output::tptp::display_function_declaration(function_declaration,
						&format_context))?;
			}
		}

		let function_declarations = self.function_declarations.borrow();
		let symbolic_constants = function_declarations.iter().filter(
			|x| !self.input_constant_declaration_domains.borrow().contains_key(*x));

		let mut last_symbolic_constant: Option<std::rc::Rc<foliage::FunctionDeclaration>> =
			None;

		for (i, symbolic_constant) in symbolic_constants.enumerate()
		{
			// Order axioms are only necessary with two or more symbolic constants
			if i == 1
			{
				writeln!(formatter, "% axioms for order of symbolic constants")?;
			}

			if symbolic_constant.arity > 0
			{
				// TODO: refactor
				unimplemented!("n-ary function declarations aren’t supported");
			}

			if let Some(last_symbolic_constant) = last_symbolic_constant
			{
				writeln!(formatter, "tff(symbolic_constant_order, axiom, p__less__({}, {})).",
					last_symbolic_constant.name, symbolic_constant.name)?;
			}

			last_symbolic_constant = Some(std::rc::Rc::clone(symbolic_constant));
		}

		for (section_kind, statements) in self.statements.borrow().iter()
		{
			if statements.is_empty()
			{
				continue;
			}

			let title = match section_kind
			{
				SectionKind::CompletedDefinitions => "completed definitions",
				SectionKind::IntegrityConstraints => "integrity constraints",
				SectionKind::Axioms => "axioms",
				SectionKind::Assumptions => "assumptions",
				SectionKind::Lemmas => "lemmas",
				SectionKind::Assertions => "assertions",
			};

			write_title(formatter, title, section_separator)?;
			section_separator = "\n";

			let mut i = 0;

			for statement in statements.iter()
			{
				if let Some(ref description) = statement.description
				{
					writeln!(formatter, "% {}", description)?;
				}

				let name = match &statement.name
				{
					// TODO: refactor
					Some(name) => name.clone(),
					None =>
					{
						i += 1;
						format!("statement_{}", i)
					},
				};

				let statement_type = match statement.proof_status
				{
					ProofStatus::AssumedProven => "axiom",
					ProofStatus::ToProveNow => "conjecture",
					// Skip statements that will be proven later
					ProofStatus::ToProveLater => continue,
					// Skip statements that are not part of this proof
					ProofStatus::Ignored => continue,
				};

				// TODO: add proper statement type
				writeln!(formatter, "tff({}, {}, {}).", name, statement_type,
					crate::output::tptp::display_formula(&statement.formula, &format_context))?;
			}
		}

		Ok(())
	}
}

impl foliage::FindOrCreateFunctionDeclaration for Problem
{
	fn find_or_create_function_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		let mut function_declarations = self.function_declarations.borrow_mut();

		match function_declarations.iter().find(|x| x.name == name && x.arity == arity)
		{
			Some(declaration) => std::rc::Rc::clone(&declaration),
			None =>
			{
				let declaration = foliage::FunctionDeclaration
				{
					name: name.to_string(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

				function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl foliage::FindOrCreatePredicateDeclaration for Problem
{
	fn find_or_create_predicate_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		let mut predicate_declarations = self.predicate_declarations.borrow_mut();

		match predicate_declarations.iter().find(|x| x.name == name && x.arity == arity)
		{
			Some(declaration) => std::rc::Rc::clone(&declaration),
			None =>
			{
				let declaration = foliage::PredicateDeclaration
				{
					name: name.to_string(),
					arity,
				};
				let declaration = std::rc::Rc::new(declaration);

				predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::traits::AssignVariableDeclarationDomain for Problem
{
	fn assign_variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>, domain: crate::Domain)
	{
		let mut variable_declaration_domains = self.variable_declaration_domains.borrow_mut();

		match variable_declaration_domains.get(variable_declaration)
		{
			Some(current_domain) => assert_eq!(*current_domain, domain,
				"inconsistent variable declaration domain"),
			None =>
			{
				variable_declaration_domains
					.insert(std::rc::Rc::clone(variable_declaration).into(), domain);
			},
		}
	}
}

impl<'a, 'b> FormatContext<'a, 'b>
{
	fn variable_declaration_id(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> usize
	{
		let mut variable_declaration_ids = match self.variable_declaration_domains
			.get(variable_declaration)
		{
			Some(crate::Domain::Program) => self.program_variable_declaration_ids.borrow_mut(),
			Some(crate::Domain::Integer) => self.integer_variable_declaration_ids.borrow_mut(),
			None => unreachable!("all variables should be declared at this point"),
		};

		match variable_declaration_ids.get(variable_declaration)
		{
			Some(id) => *id,
			None =>
			{
				let id = variable_declaration_ids.len();
				variable_declaration_ids.insert(std::rc::Rc::clone(variable_declaration).into(), id);
				id
			},
		}
	}
}

impl<'a, 'b> crate::traits::InputConstantDeclarationDomain for FormatContext<'a, 'b>
{
	fn input_constant_declaration_domain(&self,
		declaration: &std::rc::Rc<foliage::FunctionDeclaration>) -> crate::Domain
	{
		// Assume the program domain if not specified otherwise
		self.input_constant_declaration_domains.get(declaration).map(|x| *x)
			.unwrap_or(crate::Domain::Program)
	}
}

impl<'a, 'b> crate::traits::VariableDeclarationDomain for FormatContext<'a, 'b>
{
	fn variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> Option<crate::Domain>
	{
		self.variable_declaration_domains.get(variable_declaration)
			.map(|x| *x)
	}
}

impl<'a, 'b> crate::traits::VariableDeclarationID for FormatContext<'a, 'b>
{
	fn variable_declaration_id(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> usize
	{
		use crate::traits::VariableDeclarationDomain;

		let mut variable_declaration_ids = match self.variable_declaration_domain(
			variable_declaration)
		{
			Some(crate::Domain::Program) => self.program_variable_declaration_ids.borrow_mut(),
			Some(crate::Domain::Integer) => self.integer_variable_declaration_ids.borrow_mut(),
			None => panic!("all variables should be declared at this point"),
		};

		match variable_declaration_ids.get(variable_declaration)
		{
			Some(id) =>
			{
				*id
			}
			None =>
			{
				let id = variable_declaration_ids.len();
				variable_declaration_ids.insert(std::rc::Rc::clone(variable_declaration).into(), id);
				id
			},
		}
	}
}

impl<'a, 'b> foliage::format::Format for FormatContext<'a, 'b>
{
	fn display_variable_declaration(&self, formatter: &mut std::fmt::Formatter,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> std::fmt::Result
	{
		let id = self.variable_declaration_id(variable_declaration);
		let domain = self.variable_declaration_domains.get(variable_declaration)
			.expect("unspecified variable domain");

		let prefix = match domain
		{
			crate::Domain::Integer => "N",
			crate::Domain::Program => "X",
		};

		write!(formatter, "{}{}", prefix, id + 1)
	}
}

fn run_vampire<I, S>(input: &str, arguments: Option<I>)
	-> Result<(ProofResult, Option<f32>), crate::Error>
where
	I: IntoIterator<Item = S>, S: AsRef<std::ffi::OsStr>,
{
	let mut vampire = std::process::Command::new("vampire");

	let vampire = match arguments
	{
		Some(arguments) => vampire.args(arguments),
		None => &mut vampire,
	};

	let mut vampire = vampire
		.stdin(std::process::Stdio::piped())
		.stdout(std::process::Stdio::piped())
		.stderr(std::process::Stdio::piped())
		.spawn()
		.map_err(|error| crate::Error::new_run_vampire(error))?;

	{
		use std::io::Write as _;

		let vampire_stdin = vampire.stdin.as_mut().unwrap();
		vampire_stdin.write_all(input.as_bytes())
			.map_err(|error| crate::Error::new_run_vampire(error))?;
	}

	let output = vampire.wait_with_output()
		.map_err(|error| crate::Error::new_run_vampire(error))?;

	let stdout = std::str::from_utf8(&output.stdout)
		.map_err(|error| crate::Error::new_run_vampire(error))?;

	let stderr = std::str::from_utf8(&output.stderr)
		.map_err(|error| crate::Error::new_run_vampire(error))?;

	if !output.status.success()
	{
		let proof_not_found_regex = regex::Regex::new(r"% \(\d+\)Proof not found in time").unwrap();

		if proof_not_found_regex.is_match(stdout)
		{
			return Ok((ProofResult::NotProven, None));
		}

		return Err(crate::Error::new_prove_program(output.status.code(), stdout.to_string(),
			stderr.to_string()));
	}

	let proof_time_regex = regex::Regex::new(r"% \(\d+\)Success in time (\d+(?:\.\d+)?) s").unwrap();

	let proof_time = proof_time_regex.captures(stdout)
		.map(|captures| captures.get(1).unwrap().as_str().parse::<f32>().ok())
		.unwrap_or(None);

	let refutation_regex = regex::Regex::new(r"% \(\d+\)Termination reason: Refutation").unwrap();

	if refutation_regex.is_match(stdout)
	{
		return Ok((ProofResult::Proven, proof_time));
	}

	// TODO: support disproven result

	Err(crate::Error::new_parse_vampire_output(stdout.to_string(), stderr.to_string()))
}
