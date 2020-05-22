mod proof_direction;
mod section_kind;
mod statement;

pub use proof_direction::ProofDirection;
pub(crate) use section_kind::SectionKind;
pub(crate) use statement::{ProofStatus, Statement, StatementKind};

use foliage::flavor::{FunctionDeclaration as _, PredicateDeclaration as _};

#[derive(Copy, Clone, Eq, PartialEq)]
pub enum ProofResult
{
	Proven,
	NotProven,
	Disproven,
}

pub struct Problem
{
	function_declarations: std::cell::RefCell<crate::FunctionDeclarations>,
	pub predicate_declarations: std::cell::RefCell<crate::PredicateDeclarations>,

	statements: std::cell::RefCell<std::collections::BTreeMap<SectionKind, Vec<Statement>>>,

	shell: std::cell::RefCell<crate::output::Shell>,
}

impl Problem
{
	pub fn new(color_choice: crate::output::ColorChoice) -> Self
	{
		Self
		{
			function_declarations: std::cell::RefCell::new(crate::FunctionDeclarations::new()),
			predicate_declarations: std::cell::RefCell::new(crate::PredicateDeclarations::new()),

			statements: std::cell::RefCell::new(std::collections::BTreeMap::new()),

			shell: std::cell::RefCell::new(crate::output::Shell::from_stdout(color_choice)),
		}
	}

	pub(crate) fn add_statement(&self, section_kind: SectionKind, statement: Statement)
	{
		let mut statements = self.statements.borrow_mut();
		let section = statements.entry(section_kind).or_insert(vec![]);

		section.push(statement);
	}

	pub(crate) fn check_that_only_input_and_output_predicates_are_used(&self)
		-> Result<(), crate::Error>
	{
		let predicate_declarations = self.predicate_declarations.borrow();

		if predicate_declarations.iter().find(|x| *x.is_output.borrow()).is_none()
		{
			return Ok(());
		}

		// Check that only input and output predicates are used in the specification
		for predicate_declaration in predicate_declarations.iter()
		{
			if *predicate_declaration.is_input.borrow()
				|| *predicate_declaration.is_output.borrow()
				// Auxiliary predicates may occur anywhere
				|| predicate_declaration.is_built_in()
			{
				continue;
			}

			for (_, statements) in self.statements.borrow().iter()
			{
				for statement in statements
				{
					match statement.kind
					{
						crate::problem::StatementKind::CompletedDefinition(_)
						| crate::problem::StatementKind::IntegrityConstraint => continue,
						_ => (),
					}

					if crate::formula_contains_predicate(&statement.formula, predicate_declaration)
					{
						return Err(crate::Error::new_predicate_should_not_occur_in_specification(
							std::rc::Rc::clone(predicate_declaration)));
					}
				}
			}
		}

		Ok(())
	}

	pub(crate) fn restrict_to_output_predicates(&mut self) -> Result<(), crate::Error>
	{
		let predicate_declarations = self.predicate_declarations.borrow();

		// If no output statements were provided, show all predicates by default
		if predicate_declarations.iter().find(|x| *x.is_output.borrow()).is_none()
		{
			return Ok(());
		}

		let hidden_predicate_declarations = predicate_declarations.iter()
			.filter(|x| !*x.is_output.borrow() && !*x.is_input.borrow() && !x.is_built_in());

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
						&mut statement.formula, &completed_definition)?;
				}
			}
		}

		Ok(())
	}

	pub fn simplify(&mut self) -> Result<(), crate::Error>
	{
		let mut statements = self.statements.borrow_mut();

		for (_, statements) in statements.iter_mut()
		{
			for statement in statements.iter_mut()
			{
				match statement.kind
				{
					// Only simplify generated formulas
					| StatementKind::CompletedDefinition(_)
					| StatementKind::IntegrityConstraint =>
						crate::simplify(&mut statement.formula)?,
					_ => (),
				}
			}
		}

		Ok(())
	}

	fn print_step_title(&self, step_title: &str, color: &termcolor::ColorSpec)
		-> Result<(), crate::Error>
	{
		let longest_possible_key = "    Finished";

		self.shell.borrow_mut().print(
			&format!("{:>step_title_width$} ", step_title,
				step_title_width = longest_possible_key.chars().count()),
			color)?;

		Ok(())
	}

	pub fn prove(&self, proof_direction: ProofDirection) -> Result<(), crate::Error>
	{
		if proof_direction == ProofDirection::Forward
			|| proof_direction == ProofDirection::Both
		{
			self.print_step_title("Started",
				termcolor::ColorSpec::new().set_bold(true).set_fg(Some(termcolor::Color::Green)))?;
			self.shell.borrow_mut().println(
				&"verification of specification from translated program",
				&termcolor::ColorSpec::new())?;

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

			self.print_step_title("Finished", &step_title_color)?;
			println!("verification of specification from translated program");
		}

		if proof_direction == ProofDirection::Both
		{
			println!("");
		}

		if proof_direction == ProofDirection::Backward
			|| proof_direction == ProofDirection::Both
		{
			self.print_step_title("Started",
				termcolor::ColorSpec::new().set_bold(true).set_fg(Some(termcolor::Color::Green)))?;
			self.shell.borrow_mut().println(
				&"verification of translated program from specification",
				&termcolor::ColorSpec::new())?;

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
						| StatementKind::Spec =>
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

			self.print_step_title("Finished", &step_title_color)?;
			println!("verification of translated program from specification");
		}

		Ok(())
	}

	fn next_unproven_statement_do_mut<F, G>(&self, mut functor: F) -> Option<G>
	where
		F: FnMut(&mut Statement) -> G,
	{
		// TODO: properly ensure that statements are proven in the right order
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
		let display_statement = |statement: &mut Statement| -> Result<(), crate::Error>
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

			self.print_step_title(&step_title, &step_title_color)?;

			self.shell.borrow_mut().print(&format!("{}: ", statement.kind),
				&termcolor::ColorSpec::new())?;

			// TODO: only perform autonaming when necessary
			crate::autoname_variables(&mut statement.formula);

			print!("{}", statement.formula);

			Ok(())
		};

		// Show all statements that are assumed to be proven
		for (_, statements) in self.statements.borrow_mut().iter_mut()
		{
			for statement in statements.iter_mut()
				.filter(|statement| statement.proof_status == ProofStatus::AssumedProven)
			{
				display_statement(statement)?;
				println!("");
			}
		}

		loop
		{
			match self.next_unproven_statement_do_mut(
				|statement| -> Result<(), crate::Error>
				{
					statement.proof_status = ProofStatus::ToProveNow;

					print!("\x1b[s");
					display_statement(statement)?;
					print!("\x1b[u");

					use std::io::Write as _;
					std::io::stdout().flush()?;

					Ok(())
				})
			{
				Some(_) => (),
				// If there are no more unproven statements left, we’re done
				None => break,
			}

			let tptp_problem_to_prove_next_statement = format!("{}", self.display_tptp());

			log::trace!("TPTP program:\n{}", &tptp_problem_to_prove_next_statement);

			// TODO: make configurable again
			let (proof_result, proof_time_seconds) =
				run_vampire(&tptp_problem_to_prove_next_statement,
					Some(&["--mode", "casc", "--cores", "8", "--time_limit", "300"]))?;

			match self.next_unproven_statement_do_mut(
				|statement| -> Result<(), crate::Error>
				{
					statement.proof_status = match proof_result
					{
						ProofResult::Proven => ProofStatus::Proven,
						ProofResult::NotProven => ProofStatus::NotProven,
						ProofResult::Disproven => ProofStatus::Disproven,
					};

					self.shell.borrow_mut().erase_line();

					display_statement(statement)?;

					match proof_result
					{
						ProofResult::Proven => (),
						ProofResult::NotProven => print!(" (not proven)"),
						ProofResult::Disproven => print!(" (disproven)"),
					}

					if let Some(proof_time_seconds) = proof_time_seconds
					{
						self.shell.borrow_mut().print(&format!(" in {} seconds", proof_time_seconds),
							termcolor::ColorSpec::new().set_fg(Some(termcolor::Color::Black)).set_intense(true))?;
					}

					Ok(())
				})
			{
				Some(_) => (),
				None => unreachable!(),
			}

			self.shell.borrow_mut().println(&"", &termcolor::ColorSpec::new())?;

			if proof_result != ProofResult::Proven
			{
				return Ok(proof_result);
			}
		}

		Ok(ProofResult::Proven)
	}

	fn display_tptp(&self) -> ProblemTPTPDisplay
	{
		ProblemTPTPDisplay(self)
	}
}

struct ProblemTPTPDisplay<'p>(&'p Problem);

impl<'p> std::fmt::Display for ProblemTPTPDisplay<'p>
{
	fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
	{
		let write_title = |formatter: &mut dyn std::fmt::Write, title, section_separator|
			-> std::fmt::Result
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

		if !self.0.predicate_declarations.borrow().is_empty()
			|| !self.0.function_declarations.borrow().is_empty()
		{
			write_title(formatter, "types", section_separator)?;

			if !self.0.predicate_declarations.borrow().is_empty()
			{
				writeln!(formatter, "% predicate types")?;
			}

			for predicate_declaration in self.0.predicate_declarations.borrow().iter()
				.filter(|x| !x.is_built_in())
			{
				writeln!(formatter, "tff(type, type, {}).",
					crate::output::tptp::display_predicate_declaration(predicate_declaration))?;
			}

			if !self.0.function_declarations.borrow().is_empty()
			{
				writeln!(formatter, "% function types")?;
			}

			for function_declaration in self.0.function_declarations.borrow().iter()
				.filter(|x| !x.is_built_in())
			{
				writeln!(formatter, "tff(type, type, {}).",
					crate::output::tptp::display_function_declaration(function_declaration))?;
			}
		}

		let function_declarations = self.0.function_declarations.borrow();
		let symbolic_constants = function_declarations.iter().filter(|x| !*x.is_input.borrow());

		let mut last_symbolic_constant: Option<std::rc::Rc<crate::FunctionDeclaration>> = None;

		// TODO: put in axioms section
		for (i, symbolic_constant) in symbolic_constants.enumerate()
		{
			// Order axioms are only necessary with two or more symbolic constants
			if i == 1
			{
				writeln!(formatter, "% axioms for order of symbolic constants")?;
			}

			if symbolic_constant.arity() > 0
			{
				// TODO: refactor
				unimplemented!("n-ary function declarations aren’t supported");
			}

			if let Some(last_symbolic_constant) = last_symbolic_constant
			{
				write!(formatter, "tff(symbolic_constant_order, axiom, p__less__(")?;
				last_symbolic_constant.display_name(formatter)?;
				write!(formatter, ", ")?;
				symbolic_constant.display_name(formatter)?;
				writeln!(formatter, ")).")?;
			}

			last_symbolic_constant = Some(std::rc::Rc::clone(symbolic_constant));
		}

		for (section_kind, statements) in self.0.statements.borrow_mut().iter_mut()
		{
			if statements.is_empty()
			{
				continue;
			}

			// TODO: refactor
			let title = match section_kind
			{
				SectionKind::CompletedDefinitions => "completed definitions",
				SectionKind::IntegrityConstraints => "integrity constraints",
				SectionKind::Axioms => "axioms",
				SectionKind::Assumptions => "assumptions",
				SectionKind::Lemmas => "lemmas",
				SectionKind::Specs => "specs",
			};

			write_title(formatter, title, section_separator)?;
			section_separator = "\n";

			let mut i = 0;

			for statement in statements.iter_mut()
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

				// TODO: avoid doing this twice
				match statement.kind
				{
					StatementKind::CompletedDefinition(_)
					| StatementKind::IntegrityConstraint =>
						crate::autoname_variables(&mut statement.formula),
					_ => (),
				}

				// TODO: add proper statement type
				writeln!(formatter, "tff({}, {}, {}).", name, statement_type,
					crate::output::tptp::display_formula(&statement.formula))?;
			}
		}

		Ok(())
	}
}

impl foliage::FindOrCreateFunctionDeclaration<crate::FoliageFlavor> for Problem
{
	fn find_or_create_function_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<crate::FunctionDeclaration>
	{
		let mut function_declarations = self.function_declarations.borrow_mut();

		match function_declarations.iter().find(|x| x.matches_signature(name, arity))
		{
			Some(declaration) => std::rc::Rc::clone(&declaration),
			None =>
			{
				let declaration = crate::FunctionDeclaration::new(name.to_string(), arity);
				let declaration = std::rc::Rc::new(declaration);

				function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl foliage::FindOrCreatePredicateDeclaration<crate::FoliageFlavor> for Problem
{
	fn find_or_create_predicate_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<crate::PredicateDeclaration>
	{
		let mut predicate_declarations = self.predicate_declarations.borrow_mut();

		match predicate_declarations.iter().find(|x| x.matches_signature(name, arity))
		{
			Some(declaration) => std::rc::Rc::clone(&declaration),
			None =>
			{
				let declaration = crate::PredicateDeclaration::new(name.to_string(), arity);
				let declaration = std::rc::Rc::new(declaration);

				predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
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
