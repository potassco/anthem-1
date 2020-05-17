mod proof_direction;
mod section_kind;
mod statement;

pub use proof_direction::ProofDirection;
pub(crate) use section_kind::SectionKind;
pub(crate) use statement::{ProofStatus, Statement, StatementKind};

#[derive(Copy, Clone, Eq, PartialEq)]
pub enum ProofResult
{
	Proven,
	NotProven,
	Disproven,
}

type VariableDeclarationIDs
	= std::collections::BTreeMap::<std::rc::Rc<foliage::VariableDeclaration>, usize>;

type InputConstantDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::FunctionDeclaration>, crate::Domain>;

type VariableDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::VariableDeclaration>, crate::Domain>;

struct FormatContext<'a, 'b>
{
	pub program_variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
	pub integer_variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
	pub input_constant_declaration_domains: &'a InputConstantDeclarationDomains,
	pub variable_declaration_domains: &'b VariableDeclarationDomains,
}

pub struct Problem
{
	function_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,

	statements: std::cell::RefCell<std::collections::BTreeMap<SectionKind, Vec<Statement>>>,

	pub input_constant_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub input_constant_declaration_domains: std::cell::RefCell<InputConstantDeclarationDomains>,
	pub input_predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,
	pub output_predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,
	// TODO: clean up as variable declarations are dropped
	variable_declaration_domains: std::cell::RefCell<VariableDeclarationDomains>,

	shell: std::cell::RefCell<crate::output::Shell>,
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
				std::cell::RefCell::new(InputConstantDeclarationDomains::new()),
			input_predicate_declarations:
				std::cell::RefCell::new(foliage::PredicateDeclarations::new()),
			output_predicate_declarations:
				std::cell::RefCell::new(foliage::PredicateDeclarations::new()),
			variable_declaration_domains:
				std::cell::RefCell::new(VariableDeclarationDomains::new()),

			shell: std::cell::RefCell::new(crate::output::Shell::from_stdout()),
		}
	}

	pub(crate) fn add_statement(&self, section_kind: SectionKind, statement: Statement)
	{
		let mut statements = self.statements.borrow_mut();
		let section = statements.entry(section_kind).or_insert(vec![]);

		section.push(statement);
	}

	pub(crate) fn restrict_to_output_predicates(&mut self) -> Result<(), crate::Error>
	{
		let predicate_declarations = self.predicate_declarations.borrow();
		let input_predicate_declarations = self.input_predicate_declarations.borrow();
		let output_predicate_declarations = self.output_predicate_declarations.borrow();

		// If no output statements were provided, show all predicates by default
		if output_predicate_declarations.is_empty()
		{
			return Ok(());
		}

		let hidden_predicate_declarations =
			predicate_declarations.iter().filter(|x| !output_predicate_declarations.contains(*x)
				&& !input_predicate_declarations.contains(*x));

		let mut statements = self.statements.borrow_mut();

		for hidden_predicate_declaration in hidden_predicate_declarations
		{
			let matches_completed_definition =
				|(_, statement): &(_, &Statement)| match statement.kind
				{
					StatementKind::CompletedDefinition(ref predicate_declaration) =>
						predicate_declaration == hidden_predicate_declaration,
					_ => false,
				};

			let completed_definitions = match statements.get_mut(&SectionKind::CompletedDefinitions)
			{
				Some(completed_definitions) => completed_definitions,
				None => return Ok(()),
			};

			let completed_definition = match completed_definitions.iter().enumerate()
				.find(matches_completed_definition)
			{
				Some((completed_definition_index, _)) =>
					completed_definitions.remove(completed_definition_index).formula,
				None => return Err(crate::Error::new_no_completed_definition_found(
					std::rc::Rc::clone(hidden_predicate_declaration))),
			};

			drop(completed_definitions);

			for (_, statements) in statements.iter_mut()
			{
				for statement in statements.iter_mut()
				{
					crate::utils::replace_predicate_in_formula_with_completed_definition(
						&mut statement.formula, &completed_definition, self)?;
				}
			}
		}

		Ok(())
	}

	fn print_step_title(&self, step_title: &str, color: &termcolor::ColorSpec)
	{
		let longest_possible_key = "    Finished";

		self.shell.borrow_mut().print(
			&format!("{:>step_title_width$} ", step_title,
				step_title_width = longest_possible_key.chars().count()),
			color);
	}

	pub fn prove(&self, proof_direction: ProofDirection) -> Result<(), crate::Error>
	{
		if proof_direction == ProofDirection::Forward
			|| proof_direction == ProofDirection::Both
		{
			self.print_step_title("Started",
				termcolor::ColorSpec::new().set_bold(true).set_fg(Some(termcolor::Color::Green)));
			self.shell.borrow_mut().println(&"verification of assertions from translated program",
				&termcolor::ColorSpec::new());

			let mut statements = self.statements.borrow_mut();

			// Initially reset all proof statuses
			for (_, statements) in statements.iter_mut()
			{
				for statement in statements.iter_mut()
				{
					match statement.kind
					{
						StatementKind::Axiom
						| StatementKind::Assumption
						| StatementKind::CompletedDefinition(_)
						| StatementKind::IntegrityConstraint =>
							statement.proof_status = ProofStatus::AssumedProven,
						StatementKind::Lemma(ProofDirection::Backward) =>
							statement.proof_status = ProofStatus::Ignored,
						_ => statement.proof_status = ProofStatus::ToProveLater,
					}
				}
			}

			drop(statements);

			let proof_result = self.prove_unproven_statements()?;

			let mut step_title_color = termcolor::ColorSpec::new();
			step_title_color.set_bold(true);

			match proof_result
			{
				ProofResult::Proven => step_title_color.set_fg(Some(termcolor::Color::Green)),
				ProofResult::NotProven => step_title_color.set_fg(Some(termcolor::Color::Yellow)),
				ProofResult::Disproven => step_title_color.set_fg(Some(termcolor::Color::Red)),
			};

			self.print_step_title("Finished", &step_title_color);
			println!("verification of assertions from translated program");
		}

		if proof_direction == ProofDirection::Both
		{
			println!("");
		}

		if proof_direction == ProofDirection::Backward
			|| proof_direction == ProofDirection::Both
		{
			self.print_step_title("Started",
				termcolor::ColorSpec::new().set_bold(true).set_fg(Some(termcolor::Color::Green)));
			self.shell.borrow_mut().println(&"verification of translated program from assertions",
				&termcolor::ColorSpec::new());

			let mut statements = self.statements.borrow_mut();

			// Initially reset all proof statuses
			for (_, statements) in statements.iter_mut()
			{
				for statement in statements.iter_mut()
				{
					match statement.kind
					{
						StatementKind::Axiom
						| StatementKind::Assumption
						| StatementKind::Assertion =>
							statement.proof_status = ProofStatus::AssumedProven,
						StatementKind::Lemma(ProofDirection::Forward) =>
							statement.proof_status = ProofStatus::Ignored,
						_ => statement.proof_status = ProofStatus::ToProveLater,
					}
				}
			}

			drop(statements);

			let proof_result = self.prove_unproven_statements()?;

			let mut step_title_color = termcolor::ColorSpec::new();
			step_title_color.set_bold(true);

			match proof_result
			{
				ProofResult::Proven => step_title_color.set_fg(Some(termcolor::Color::Green)),
				ProofResult::NotProven => step_title_color.set_fg(Some(termcolor::Color::Yellow)),
				ProofResult::Disproven => step_title_color.set_fg(Some(termcolor::Color::Red)),
			};

			self.print_step_title("Finished", &step_title_color);
			println!("verification of translated program from assertions");
		}

		Ok(())
	}

	fn next_unproven_statement_do_mut<F, G>(&self, mut functor: F) -> Option<G>
	where
		F: FnMut(&mut Statement) -> G,
	{
		for section in self.statements.borrow_mut().iter_mut()
		{
			for statement in section.1.iter_mut()
			{
				if statement.proof_status == ProofStatus::ToProveNow
					|| statement.proof_status == ProofStatus::ToProveLater
				{
					return Some(functor(statement));
				}
			}
		}

		None
	}

	fn prove_unproven_statements(&self) -> Result<ProofResult, crate::Error>
	{
		let display_statement = |statement: &Statement|
		{
			let step_title = match statement.proof_status
			{
				ProofStatus::AssumedProven => format!("Added"),
				ProofStatus::Proven => format!("Verified"),
				ProofStatus::NotProven
				| ProofStatus::Disproven
				| ProofStatus::ToProveNow => format!("Verifying"),
				ProofStatus::ToProveLater => format!("Skipped"),
				ProofStatus::Ignored => format!("Ignored"),
			};

			let mut step_title_color = termcolor::ColorSpec::new();
			step_title_color.set_bold(true);

			match statement.proof_status
			{
				ProofStatus::AssumedProven
				| ProofStatus::Proven => step_title_color.set_fg(Some(termcolor::Color::Green)),
				ProofStatus::NotProven => step_title_color.set_fg(Some(termcolor::Color::Yellow)),
				ProofStatus::Disproven => step_title_color.set_fg(Some(termcolor::Color::Red)),
				_ => step_title_color.set_fg(Some(termcolor::Color::Cyan)),
			};

			self.print_step_title(&step_title, &step_title_color);

			self.shell.borrow_mut().print(&format!("{}: ", statement.kind),
				&termcolor::ColorSpec::new());

			let format_context = FormatContext
			{
				program_variable_declaration_ids:
					std::cell::RefCell::new(VariableDeclarationIDs::new()),
				integer_variable_declaration_ids:
					std::cell::RefCell::new(VariableDeclarationIDs::new()),
				input_constant_declaration_domains: &self.input_constant_declaration_domains.borrow(),
				variable_declaration_domains: &self.variable_declaration_domains.borrow(),
			};

			print!("{}", foliage::format::display_formula(&statement.formula, &format_context));
		};

		// Show all statements that are assumed to be proven
		for (_, statements) in self.statements.borrow_mut().iter_mut()
		{
			for statement in statements.iter_mut()
				.filter(|statement| statement.proof_status == ProofStatus::AssumedProven)
			{
				display_statement(statement);
				println!("");
			}
		}

		loop
		{
			match self.next_unproven_statement_do_mut(
				|statement|
				{
					statement.proof_status = ProofStatus::ToProveNow;

					print!("\x1b[s");
					display_statement(statement);
					print!("\x1b[u");

					use std::io::Write as _;
					std::io::stdout().flush();
				})
			{
				Some(_) => (),
				// If there are no more unproven statements left, we’re done
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

			match self.next_unproven_statement_do_mut(
				|statement|
				{
					statement.proof_status = match proof_result
					{
						ProofResult::Proven => ProofStatus::Proven,
						ProofResult::NotProven => ProofStatus::NotProven,
						ProofResult::Disproven => ProofStatus::Disproven,
					};

					self.shell.borrow_mut().erase_line();

					display_statement(statement);

					match proof_result
					{
						ProofResult::Proven => (),
						ProofResult::NotProven => print!(" (not proven)"),
						ProofResult::Disproven => print!(" (disproven)"),
					}

					if let Some(proof_time_seconds) = proof_time_seconds
					{
						self.shell.borrow_mut().print(&format!(" in {} seconds", proof_time_seconds),
							termcolor::ColorSpec::new().set_fg(Some(termcolor::Color::Black)).set_intense(true));
					}
				})
			{
				Some(_) => (),
				None => unreachable!(),
			}

			self.shell.borrow_mut().println(&"", &termcolor::ColorSpec::new());

			if proof_result != ProofResult::Proven
			{
				return Ok(proof_result);
			}
		}

		Ok(ProofResult::Proven)
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

		// TODO: put in axioms section
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
				if let StatementKind::CompletedDefinition(_) = statement.kind
				{
					writeln!(formatter, "% {}", statement.kind)?;
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
					ProofStatus::AssumedProven
					| ProofStatus::Proven => "axiom",
					ProofStatus::NotProven
					| ProofStatus::Disproven => unreachable!(),
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

impl crate::traits::VariableDeclarationDomain for Problem
{
	fn variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> Option<crate::Domain>
	{
		self.variable_declaration_domains.borrow().iter().find_map(
			|(x, domain)|
			match x == variable_declaration
			{
				true => Some(*domain),
				false => None,
			})
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
