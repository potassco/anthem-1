mod head_type;
mod translate_body;

use head_type::*;
use translate_body::*;

struct ScopedFormula
{
	free_variable_declarations: foliage::VariableDeclarations,
	formula: foliage::Formula,
}

struct Definitions
{
	head_atom_parameters: std::rc::Rc<foliage::VariableDeclarations>,
	definitions: Vec<ScopedFormula>,
}

struct Context
{
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

pub fn translate(program: &str) -> i32
{
	let mut statement_handler = StatementHandler::new();

	match clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)
	{
		Ok(()) => 0,
		Err(error) =>
		{
			log::error!("could not translate input program: {}", error);
			1
		},
	}
}

fn read_rule(rule: &clingo::ast::Rule, context: &mut Context) -> Result<(), crate::Error>
{
	use super::common::FindOrCreatePredicateDeclaration;

	let head_type = determine_head_type(rule.head(),
		|name, arity| context.predicate_declarations.find_or_create(name, arity))?;

	let mut definitions
		= std::collections::BTreeMap::<std::rc::Rc<foliage::PredicateDeclaration>, Definitions>::new();

	let declare_predicate_parameters = |predicate_declaration: &foliage::PredicateDeclaration|
	{
		std::rc::Rc::new((0..predicate_declaration.arity)
			.map(|_| std::rc::Rc::new(foliage::VariableDeclaration::new("<anonymous>".to_string())))
			.collect())
	};

	match head_type
	{
		HeadType::SingleAtom(head_atom) =>
		{
			if !definitions.contains_key(&head_atom.predicate_declaration)
			{
				definitions.insert(std::rc::Rc::clone(&head_atom.predicate_declaration),
					Definitions
					{
						head_atom_parameters: declare_predicate_parameters(&head_atom.predicate_declaration),
						definitions: vec![],
					});
			}

			let definitions = definitions.get_mut(&head_atom.predicate_declaration).unwrap();

			context.variable_declaration_stack.push(std::rc::Rc::clone(
				&definitions.head_atom_parameters));

			let mut definition_arguments = translate_body(rule.body(),
				&mut context.function_declarations, &mut context.predicate_declarations,
				&mut context.variable_declaration_stack)?;

			assert_eq!(definitions.head_atom_parameters.len(), head_atom.arguments.len());

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

			let definition = foliage::Formula::And(definition_arguments);

			let definition = ScopedFormula
			{
				free_variable_declarations,
				formula: definition,
			};

			log::trace!("translated rule with single atom in head: {:?}", definition.formula);

			definitions.definitions.push(definition);
		},
		HeadType::ChoiceWithSingleAtom(_) =>
			log::debug!("translating choice rule with single atom"),
		HeadType::IntegrityConstraint =>
			log::debug!("translated integrity constraint"),
		HeadType::Trivial => log::debug!("skipping trivial rule"),
	}

	Ok(())
}
