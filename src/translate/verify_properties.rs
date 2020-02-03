mod head_type;
mod translate_body;

use head_type::*;
use translate_body::*;
use crate::translate::common::AssignVariableDeclarationDomain as _;

struct ScopedFormula
{
	free_variable_declarations: std::rc::Rc<foliage::VariableDeclarations>,
	formula: Box<foliage::Formula>,
}

struct Definitions
{
	head_atom_parameters: std::rc::Rc<foliage::VariableDeclarations>,
	definitions: Vec<ScopedFormula>,
}

type VariableDeclarationDomains
	= std::collections::BTreeMap<std::rc::Rc<foliage::VariableDeclaration>,
		crate::translate::common::Domain>;

type VariableDeclarationIDs
	= std::collections::BTreeMap::<std::rc::Rc<foliage::VariableDeclaration>, usize>;

struct Context
{
	pub definitions: std::cell::RefCell<std::collections::BTreeMap::<std::rc::Rc<foliage::PredicateDeclaration>,
		Definitions>>,
	pub integrity_constraints: std::cell::RefCell<foliage::Formulas>,

	pub function_declarations: std::cell::RefCell<foliage::FunctionDeclarations>,
	pub predicate_declarations: std::cell::RefCell<foliage::PredicateDeclarations>,
	pub variable_declaration_stack: std::cell::RefCell<foliage::VariableDeclarationStack>,
	pub variable_declaration_domains: std::cell::RefCell<VariableDeclarationDomains>,
	pub variable_declaration_ids: std::cell::RefCell<VariableDeclarationIDs>,
}

impl Context
{
	fn new() -> Self
	{
		Self
		{
			definitions: std::cell::RefCell::new(std::collections::BTreeMap::<_, _>::new()),
			integrity_constraints: std::cell::RefCell::new(vec![]),

			function_declarations: std::cell::RefCell::new(foliage::FunctionDeclarations::new()),
			predicate_declarations: std::cell::RefCell::new(foliage::PredicateDeclarations::new()),
			variable_declaration_stack: std::cell::RefCell::new(foliage::VariableDeclarationStack::new()),
			variable_declaration_domains: std::cell::RefCell::new(VariableDeclarationDomains::new()),
			variable_declaration_ids: std::cell::RefCell::new(VariableDeclarationIDs::new()),
		}
	}
}

impl crate::translate::common::GetOrCreateFunctionDeclaration for Context
{
	fn get_or_create_function_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::FunctionDeclaration>
	{
		let mut function_declarations = self.function_declarations.borrow_mut();

		match function_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::FunctionDeclaration::new(
					name.to_string(), arity));

				function_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new function declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::translate::common::GetOrCreatePredicateDeclaration for Context
{
	fn get_or_create_predicate_declaration(&self, name: &str, arity: usize)
		-> std::rc::Rc<foliage::PredicateDeclaration>
	{
		let mut predicate_declarations = self.predicate_declarations.borrow_mut();

		match predicate_declarations.iter()
			.find(|x| x.name == name && x.arity == arity)
		{
			Some(value) => std::rc::Rc::clone(value),
			None =>
			{
				let declaration = std::rc::Rc::new(foliage::PredicateDeclaration::new(
					name.to_string(), arity));

				predicate_declarations.insert(std::rc::Rc::clone(&declaration));

				log::debug!("new predicate declaration: {}/{}", name, arity);

				declaration
			},
		}
	}
}

impl crate::translate::common::GetOrCreateVariableDeclaration for Context
{
	fn get_or_create_variable_declaration(&self, name: &str)
		-> std::rc::Rc<foliage::VariableDeclaration>
	{
		let mut variable_declaration_stack = self.variable_declaration_stack.borrow_mut();

		// TODO: check correctness
		if name == "_"
		{
			let variable_declaration = std::rc::Rc::new(foliage::VariableDeclaration::new(
				"_".to_owned()));

			variable_declaration_stack.free_variable_declarations.push(
				std::rc::Rc::clone(&variable_declaration));

			return variable_declaration;
		}

		variable_declaration_stack.find_or_create(name)
	}
}

impl crate::translate::common::AssignVariableDeclarationDomain for Context
{
	fn assign_variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>,
		domain: crate::translate::common::Domain)
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

impl crate::translate::common::VariableDeclarationDomain for Context
{
	fn variable_declaration_domain(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> Option<crate::translate::common::Domain>
	{
		let variable_declaration_domains = self.variable_declaration_domains.borrow();

		variable_declaration_domains.get(variable_declaration)
			.map(|x| *x)
	}
}

impl crate::translate::common::VariableDeclarationID for Context
{
	fn variable_declaration_id(&self,
		variable_declaration: &std::rc::Rc<foliage::VariableDeclaration>)
		-> usize
	{
		let mut variable_declaration_ids = self.variable_declaration_ids.borrow_mut();

		match variable_declaration_ids.get(variable_declaration)
		{
			Some(id) =>
			{
				//log::trace!("{:p} → {} (already known)", *variable_declaration, id);
				*id
			}
			None =>
			{
				let id = variable_declaration_ids.len();
				variable_declaration_ids.insert(std::rc::Rc::clone(variable_declaration).into(), id);
				//log::trace!("{:p} → {} (new)", *variable_declaration, id);
				id
			},
		}
	}
}

struct StatementHandler
{
	context: Context,
}

impl StatementHandler
{
	fn new() -> Self
	{
		Self
		{
			context: Context::new(),
		}
	}
}

impl clingo::StatementHandler for StatementHandler
{
	fn on_statement(&mut self, statement: &clingo::ast::Statement) -> bool
	{
		match statement.statement_type()
		{
			clingo::ast::StatementType::Rule(ref rule) =>
			{
				if let Err(error) = read_rule(rule, &mut self.context)
				{
					log::error!("could not translate input program: {}", error);
					return false;
				}
			},
			_ => log::debug!("read statement (other kind)"),
		}

		true
	}
}

struct Logger;

impl clingo::Logger for Logger
{
	fn log(&mut self, code: clingo::Warning, message: &str)
	{
		log::warn!("clingo warning ({:?}): {}", code, message);
	}
}

pub fn translate<P>(program_paths: &[P]) -> Result<(), crate::Error>
where
	P: AsRef<std::path::Path>
{
	let mut statement_handler = StatementHandler::new();

	for program_path in program_paths
	{
		let program = std::fs::read_to_string(program_path.as_ref())
			.map_err(|error| crate::Error::new_read_file(program_path.as_ref().to_path_buf(), error))?;

		clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)
			.map_err(|error| crate::Error::new_translate(error))?;
	}

	let context = statement_handler.context;

	for (predicate_declaration, definitions) in context.definitions.borrow().iter()
	{
		for definition in definitions.definitions.iter()
		{
			log::debug!("definition({}/{}): {}.", predicate_declaration.name, predicate_declaration.arity,
				definition.formula);
		}
	}

	let completed_definition = |predicate_declaration|
	{
		match context.definitions.borrow_mut().remove(predicate_declaration)
		{
			// This predicate symbol has at least one definition, so build the disjunction of those
			Some(definitions) =>
			{
				let or_arguments = definitions.definitions.into_iter()
					.map(|x| existential_closure(x))
					.collect::<Vec<_>>();
				let or = foliage::Formula::or(or_arguments);

				let head_arguments = definitions.head_atom_parameters.iter()
					.map(|x| Box::new(foliage::Term::variable(x)))
					.collect::<Vec<_>>();

				let head_predicate = foliage::Formula::predicate(&predicate_declaration,
					head_arguments);

				let completed_definition = foliage::Formula::if_and_only_if(
					Box::new(head_predicate), Box::new(or));

				let scoped_formula = ScopedFormula
				{
					free_variable_declarations: definitions.head_atom_parameters,
					formula: Box::new(completed_definition),
				};

				universal_closure(scoped_formula)
			},
			// This predicate has no definitions, so universally falsify it
			None =>
			{
				let head_atom_parameters = std::rc::Rc::new((0..predicate_declaration.arity)
					.map(|_|
					{
						let variable_declaration = std::rc::Rc::new(
							foliage::VariableDeclaration::new("<anonymous>".to_string()));
						context.assign_variable_declaration_domain(&variable_declaration,
							crate::translate::common::Domain::Program);
						variable_declaration
					})
					.collect::<Vec<_>>());

				let head_arguments = head_atom_parameters.iter()
					.map(|x| Box::new(foliage::Term::variable(x)))
					.collect();

				let head_predicate = foliage::Formula::predicate(&predicate_declaration,
					head_arguments);

				let not = foliage::Formula::not(Box::new(head_predicate));

				let scoped_formula = ScopedFormula
				{
					free_variable_declarations: head_atom_parameters,
					formula: Box::new(not),
				};

				universal_closure(scoped_formula)
			},
		}
	};

	let predicate_declarations = context.predicate_declarations.borrow();
	let completed_definitions = predicate_declarations.iter()
		.map(|x| (std::rc::Rc::clone(x), completed_definition(x)));

	for (predicate_declaration, completed_definition) in completed_definitions
	{
		println!("tff(completion_{}_{}, axiom, {}).", predicate_declaration.name,
			predicate_declaration.arity,
			crate::output::tptp::display_formula(&completed_definition, &context));
	}

	for integrity_constraint in context.integrity_constraints.borrow().iter()
	{
		println!("tff(integrity_constraint, axiom, {}).",
			crate::output::tptp::display_formula(&integrity_constraint, &context));
	}

	Ok(())
}

fn read_rule(rule: &clingo::ast::Rule, context: &Context)
	-> Result<(), crate::Error>
{
	let head_type = determine_head_type(rule.head(), context)?;

	match &head_type
	{
		HeadType::SingleAtom(head_atom)
		| HeadType::ChoiceWithSingleAtom(head_atom) =>
		{
			if !context.definitions.borrow().contains_key(&head_atom.predicate_declaration)
			{
				let head_atom_parameters = std::rc::Rc::new((0..head_atom.predicate_declaration.arity)
					.map(|_|
					{
						let variable_declaration = std::rc::Rc::new(
							foliage::VariableDeclaration::new("<anonymous>".to_string()));
						context.assign_variable_declaration_domain(&variable_declaration,
							crate::translate::common::Domain::Program);
						variable_declaration
					})
					.collect());

				context.definitions.borrow_mut().insert(
					std::rc::Rc::clone(&head_atom.predicate_declaration),
					Definitions
					{
						head_atom_parameters,
						definitions: vec![],
					});
			}

			let mut definitions = context.definitions.borrow_mut();
			let definitions = definitions.get_mut(&head_atom.predicate_declaration).unwrap();

			let head_atom_parameters = std::rc::Rc::clone(&definitions.head_atom_parameters);
			context.variable_declaration_stack.borrow_mut().push(head_atom_parameters);

			let mut definition_arguments = translate_body(rule.body(), context)?;

			assert_eq!(definitions.head_atom_parameters.len(), head_atom.arguments.len());

			if let HeadType::ChoiceWithSingleAtom(_) = head_type
			{
				let head_arguments = definitions.head_atom_parameters.iter()
					.map(|x| Box::new(foliage::Term::variable(x)))
					.collect::<Vec<_>>();

				let head_predicate = foliage::Formula::predicate(&head_atom.predicate_declaration,
					head_arguments);

				definition_arguments.push(Box::new(head_predicate));
			}

			let mut head_atom_arguments_iterator = head_atom.arguments.iter();

			for head_atom_parameter in definitions.head_atom_parameters.iter()
			{
				let head_atom_argument = head_atom_arguments_iterator.next().unwrap();

				let translated_head_term = crate::translate::common::choose_value_in_term(
					head_atom_argument, head_atom_parameter, context)?;

				definition_arguments.push(Box::new(translated_head_term));
			}

			context.variable_declaration_stack.borrow_mut().pop();

			let mut free_variable_declarations = vec![];

			std::mem::swap(
				&mut context.variable_declaration_stack.borrow_mut().free_variable_declarations,
				&mut free_variable_declarations);

			let definition = match definition_arguments.len()
			{
				1 => definition_arguments.pop().unwrap(),
				0 => Box::new(foliage::Formula::true_()),
				_ => Box::new(foliage::Formula::and(definition_arguments)),
			};

			let definition = ScopedFormula
			{
				free_variable_declarations: std::rc::Rc::new(free_variable_declarations),
				formula: definition,
			};

			log::debug!("translated rule with single atom in head: {:?}", definition.formula);

			definitions.definitions.push(definition);
		},
		HeadType::IntegrityConstraint =>
		{
			let mut arguments = translate_body(rule.body(), context)?;

			let mut free_variable_declarations = vec![];

			std::mem::swap(
				&mut context.variable_declaration_stack.borrow_mut().free_variable_declarations,
				&mut free_variable_declarations);

			let formula = match arguments.len()
			{
				1 => foliage::Formula::not(arguments.pop().unwrap()),
				0 => foliage::Formula::false_(),
				_ => foliage::Formula::not(Box::new(foliage::Formula::and(arguments))),
			};

			let scoped_formula = ScopedFormula
			{
				free_variable_declarations: std::rc::Rc::new(free_variable_declarations),
				formula: Box::new(formula),
			};

			let integrity_constraint = universal_closure(scoped_formula);

			log::debug!("translated integrity constraint: {:?}", integrity_constraint);

			context.integrity_constraints.borrow_mut().push(integrity_constraint);
		},
		HeadType::Trivial => log::info!("skipping trivial rule"),
	}

	Ok(())
}

fn existential_closure(scoped_formula: ScopedFormula) -> Box<foliage::Formula>
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => Box::new(foliage::Formula::exists(scoped_formula.free_variable_declarations,
			scoped_formula.formula)),
	}
}

fn universal_closure(scoped_formula: ScopedFormula) -> Box<foliage::Formula>
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => Box::new(foliage::Formula::for_all(scoped_formula.free_variable_declarations,
			scoped_formula.formula)),
	}
}
