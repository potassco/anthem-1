pub enum StatementKind
{
	Axiom,
	Assumption,
	Lemma(crate::ProofDirection),
	Assertion,
}

enum ProofStatus
{
	AssumedProven,
	ToProve,
}

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
			proof_status: ProofStatus::ToProve,
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

	pub fn prove(&self, proof_direction: crate::ProofDirection)
	{
		if proof_direction == crate::ProofDirection::Forward
			|| proof_direction == crate::ProofDirection::Both
		{
			log::info!("performing forward proof");

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
							=> statement.proof_status = ProofStatus::AssumedProven,
						_ => statement.proof_status = ProofStatus::ToProve,
					}
				}
			}

			drop(statements);

			self.display(crate::output::Format::HumanReadable);

			log::info!("finished forward proof");
		}
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
					print!("tff({}, <TODO>, ", name);
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
