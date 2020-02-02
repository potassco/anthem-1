mod head_type;
mod translate_body;

use head_type::*;
use translate_body::*;

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

struct Context
{
	pub definitions: std::collections::BTreeMap::<std::rc::Rc<foliage::PredicateDeclaration>, Definitions>,
	pub integrity_constraints: foliage::Formulas,

	pub function_declarations: foliage::FunctionDeclarations,
	pub predicate_declarations: foliage::PredicateDeclarations,
	pub variable_declaration_stack: foliage::VariableDeclarationStack,
}

impl Context
{
	fn new() -> Self
	{
		Self
		{
			definitions: std::collections::BTreeMap::<_, _>::new(),
			integrity_constraints: vec![],
			function_declarations: foliage::FunctionDeclarations::new(),
			predicate_declarations: foliage::PredicateDeclarations::new(),
			variable_declaration_stack: foliage::VariableDeclarationStack::new(),
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

pub fn translate(program: &str) -> Result<(), crate::Error>
{
	let mut statement_handler = StatementHandler::new();

	clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)
		.map_err(|error| crate::Error::new_translate(error))?;

	let context = statement_handler.context;
	let mut definitions = context.definitions;
	let integrity_constraints = context.integrity_constraints;
	let predicate_declarations = context.predicate_declarations;

	for (predicate_declaration, definitions) in definitions.iter()
	{
		for definition in definitions.definitions.iter()
		{
			log::debug!("definition({}/{}): {}.", predicate_declaration.name, predicate_declaration.arity,
				definition.formula);
		}
	}

	let mut completed_definition = |predicate_declaration|
	{
		match definitions.remove(predicate_declaration)
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
					.map(|_| std::rc::Rc::new(foliage::VariableDeclaration::new("<anonymous>".to_string())))
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

	let completed_definitions = predicate_declarations.iter()
		.map(|x| (std::rc::Rc::clone(x), completed_definition(x)));

	for (predicate_declaration, completed_definition) in completed_definitions
	{
		println!("completion({}/{}): {}.", predicate_declaration.name,
			predicate_declaration.arity, completed_definition);
	}

	for integrity_constraint in integrity_constraints
	{
		println!("axiom: {}.", integrity_constraint);
	}

	Ok(())
}

fn read_rule(rule: &clingo::ast::Rule, context: &mut Context) -> Result<(), crate::Error>
{
	use super::common::FindOrCreatePredicateDeclaration;

	let head_type = determine_head_type(rule.head(),
		|name, arity| context.predicate_declarations.find_or_create(name, arity))?;

	match &head_type
	{
		HeadType::SingleAtom(head_atom)
		| HeadType::ChoiceWithSingleAtom(head_atom) =>
		{
			if !context.definitions.contains_key(&head_atom.predicate_declaration)
			{
				let head_atom_parameters = std::rc::Rc::new((0..head_atom.predicate_declaration.arity)
					.map(|_| std::rc::Rc::new(foliage::VariableDeclaration::new("<anonymous>".to_string())))
					.collect());

				context.definitions.insert(std::rc::Rc::clone(&head_atom.predicate_declaration),
					Definitions
					{
						head_atom_parameters,
						definitions: vec![],
					});
			}

			let definitions = context.definitions.get_mut(&head_atom.predicate_declaration).unwrap();

			context.variable_declaration_stack.push(std::rc::Rc::clone(
				&definitions.head_atom_parameters));

			let mut definition_arguments = translate_body(rule.body(),
				&mut context.function_declarations, &mut context.predicate_declarations,
				&mut context.variable_declaration_stack)?;

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
					head_atom_argument, head_atom_parameter, &mut context.function_declarations,
					&mut context.variable_declaration_stack)?;

				definition_arguments.push(Box::new(translated_head_term));
			}

			context.variable_declaration_stack.pop();

			let mut free_variable_declarations = vec![];

			std::mem::swap(&mut context.variable_declaration_stack.free_variable_declarations,
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
			let mut arguments = translate_body(rule.body(),
				&mut context.function_declarations, &mut context.predicate_declarations,
				&mut context.variable_declaration_stack)?;

			let mut free_variable_declarations = vec![];

			std::mem::swap(&mut context.variable_declaration_stack.free_variable_declarations,
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

			context.integrity_constraints.push(integrity_constraint);
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
