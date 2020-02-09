mod context;
mod head_type;
mod translate_body;

use context::*;
use head_type::*;
use translate_body::*;
use crate::traits::AssignVariableDeclarationDomain as _;

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
				if let Err(error) = read_rule(rule, &self.context)
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

pub fn translate<P>(program_paths: &[P],
	input_predicate_declarations: foliage::PredicateDeclarations,
	input_constant_declaration_domains: crate::InputConstantDeclarationDomains,
	output_format: crate::output::Format)
	-> Result<(), crate::Error>
where
	P: AsRef<std::path::Path>
{
	let mut statement_handler = StatementHandler::new();

	*statement_handler.context.input_predicate_declarations.borrow_mut()
		= input_predicate_declarations.clone();
	*statement_handler.context.predicate_declarations.borrow_mut()
		= input_predicate_declarations;
	*statement_handler.context.function_declarations.borrow_mut()
		= input_constant_declaration_domains.keys().map(std::rc::Rc::clone).collect();
	*statement_handler.context.input_constant_declaration_domains.borrow_mut()
		= input_constant_declaration_domains;

	for input_predicate_declaration in statement_handler.context.input_predicate_declarations
		.borrow().iter()
	{
		log::info!("input predicate: {}/{}", input_predicate_declaration.name,
			input_predicate_declaration.arity);
	}

	for (input_constant_declaration, domain) in statement_handler.context
		.input_constant_declaration_domains.borrow().iter()
	{
		log::info!("input constant: {} (domain: {:?})", input_constant_declaration.name, domain);
	}

	// Read all input programs
	for program_path in program_paths
	{
		let program = std::fs::read_to_string(program_path.as_ref())
			.map_err(|error| crate::Error::new_read_file(program_path.as_ref().to_path_buf(), error))?;

		clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)
			.map_err(|error| crate::Error::new_translate(error))?;

		log::info!("read input program “{}”", program_path.as_ref().display());
	}

	let context = statement_handler.context;

	for (predicate_declaration, definitions) in context.definitions.borrow().iter()
	{
		for definition in definitions.definitions.iter()
		{
			log::debug!("definition({}/{}): {}.", predicate_declaration.name, predicate_declaration.arity,
				crate::output::human_readable::display_formula(&definition.formula, None, &context));
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

				let scoped_formula = crate::ScopedFormula
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
							crate::Domain::Program);
						variable_declaration
					})
					.collect::<Vec<_>>());

				let head_arguments = head_atom_parameters.iter()
					.map(|x| Box::new(foliage::Term::variable(x)))
					.collect();

				let head_predicate = foliage::Formula::predicate(&predicate_declaration,
					head_arguments);

				let not = foliage::Formula::not(Box::new(head_predicate));

				let scoped_formula = crate::ScopedFormula
				{
					free_variable_declarations: head_atom_parameters,
					formula: Box::new(not),
				};

				universal_closure(scoped_formula)
			},
		}
	};

	let predicate_declarations = context.predicate_declarations.borrow();
	let function_declarations = context.function_declarations.borrow();
	let input_constant_declaration_domains = context.input_constant_declaration_domains.borrow();
	let completed_definitions = predicate_declarations.iter()
		// Don’t perform completion for any input predicate
		.filter(|x| !context.input_predicate_declarations.borrow().contains(*x))
		.map(|x| (std::rc::Rc::clone(x), completed_definition(x)));

	// Earlier log messages may have assigned IDs to the variable declarations, so reset them
	context.program_variable_declaration_ids.borrow_mut().clear();
	context.integer_variable_declaration_ids.borrow_mut().clear();

	let print_title = |title, section_separator|
	{
		print!("{}{}", section_separator, "%".repeat(80));
		print!("\n% {}", title);
		println!("\n{}", "%".repeat(80));
	};

	let print_formula = |formula: &foliage::Formula|
	{
		match output_format
		{
			crate::output::Format::HumanReadable => print!("{}",
				crate::output::human_readable::display_formula(formula, None, &context)),
			crate::output::Format::TPTP => print!("{}",
				crate::output::tptp::display_formula(formula, &context)),
		}
	};

	let mut section_separator = "";

	if output_format == crate::output::Format::TPTP
	{
		print_title("anthem types", section_separator);
		section_separator = "\n";

		let tptp_preamble_anthem_types
			= include_str!("../output/tptp/preamble_types.tptp").trim_end();
		println!("{}", tptp_preamble_anthem_types);

		print_title("anthem axioms", section_separator);

		let tptp_preamble_anthem_types
			= include_str!("../output/tptp/preamble_axioms.tptp").trim_end();
		println!("{}", tptp_preamble_anthem_types);

		if !predicate_declarations.is_empty() || !function_declarations.is_empty()
		{
			print_title("types", section_separator);

			if !predicate_declarations.is_empty()
			{
				println!("% predicate types")
			}

			for predicate_declaration in &*predicate_declarations
			{
				println!("tff(type, type, {}).",
					crate::output::tptp::display_predicate_declaration(predicate_declaration));
			}

			if !function_declarations.is_empty()
			{
				println!("% function types")
			}

			for function_declaration in &*function_declarations
			{
				println!("tff(type, type, {}).",
					crate::output::tptp::display_function_declaration(function_declaration,
						&context));
			}
		}

		let symbolic_constants = function_declarations.iter().filter(
			|x| !input_constant_declaration_domains.contains_key(*x));

		let mut last_symbolic_constant: Option<std::rc::Rc<foliage::FunctionDeclaration>> = None;

		for (i, symbolic_constant) in symbolic_constants.enumerate()
		{
			// Order axioms are only necessary with two or more symbolic constants
			if i == 1
			{
				println!("% axioms for order of symbolic constants")
			}

			if symbolic_constant.arity > 0
			{
				return Err(crate::Error::new_logic("unexpected n-ary function declaration"));
			}

			if let Some(last_symbolic_constant) = last_symbolic_constant
			{
				println!("tff(symbolic_constant_order, axiom, p__less__({}, {})).",
					last_symbolic_constant.name, symbolic_constant.name);
			}

			last_symbolic_constant = Some(std::rc::Rc::clone(symbolic_constant));
		}
	}

	for (i, (predicate_declaration, completed_definition)) in completed_definitions.enumerate()
	{
		if i == 0
		{
			print_title("completed definitions", section_separator);
		}

		section_separator = "\n";

		println!("% completed definition of {}/{}", predicate_declaration.name,
			predicate_declaration.arity);

		if output_format == crate::output::Format::TPTP
		{
			print!("tff(completion_{}_{}, axiom, ", predicate_declaration.name,
				predicate_declaration.arity);
		}

		print_formula(&completed_definition);

		if output_format == crate::output::Format::TPTP
		{
			print!(").");
		}

		println!("");
	}

	for (i, integrity_constraint) in context.integrity_constraints.borrow().iter().enumerate()
	{
		if i == 0
		{
			print_title("integrity constraints", section_separator);
		}

		section_separator = "\n";

		if output_format == crate::output::Format::TPTP
		{
			print!("tff(integrity_constraint, axiom, ");
		}

		print_formula(&integrity_constraint);

		if output_format == crate::output::Format::TPTP
		{
			print!(").");
		}

		println!("");
	}

	Ok(())
}

fn read_rule(rule: &clingo::ast::Rule, context: &Context) -> Result<(), crate::Error>
{
	if !context.variable_declaration_stack.borrow().is_empty()
	{
		return Err(crate::Error::new_logic("variable declaration stack in unexpected state"));
	}

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
							crate::Domain::Program);
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

			context.variable_declaration_stack.borrow_mut().pop()?;

			let free_variable_declarations = std::mem::replace(
				&mut context.variable_declaration_stack.borrow_mut().free_variable_declarations,
				vec![]);

			let definition = match definition_arguments.len()
			{
				1 => definition_arguments.pop().unwrap(),
				0 => Box::new(foliage::Formula::true_()),
				_ => Box::new(foliage::Formula::and(definition_arguments)),
			};

			let definition = crate::ScopedFormula
			{
				free_variable_declarations: std::rc::Rc::new(free_variable_declarations),
				formula: definition,
			};

			log::debug!("translated rule with single atom in head: {}",
				crate::output::human_readable::display_formula(&definition.formula, None, context));

			definitions.definitions.push(definition);
		},
		HeadType::IntegrityConstraint =>
		{
			let mut arguments = translate_body(rule.body(), context)?;

			let free_variable_declarations = std::mem::replace(
				&mut context.variable_declaration_stack.borrow_mut().free_variable_declarations,
				vec![]);

			let formula = match arguments.len()
			{
				1 => foliage::Formula::not(arguments.pop().unwrap()),
				0 => foliage::Formula::false_(),
				_ => foliage::Formula::not(Box::new(foliage::Formula::and(arguments))),
			};

			let scoped_formula = crate::ScopedFormula
			{
				free_variable_declarations: std::rc::Rc::new(free_variable_declarations),
				formula: Box::new(formula),
			};

			let integrity_constraint = universal_closure(scoped_formula);

			log::debug!("translated integrity constraint: {}",
				crate::output::human_readable::display_formula(&integrity_constraint, None,
					context));

			context.integrity_constraints.borrow_mut().push(integrity_constraint);
		},
		HeadType::Trivial => log::info!("skipping trivial rule"),
	}

	Ok(())
}

fn existential_closure(scoped_formula: crate::ScopedFormula) -> Box<foliage::Formula>
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => Box::new(foliage::Formula::exists(scoped_formula.free_variable_declarations,
			scoped_formula.formula)),
	}
}

fn universal_closure(scoped_formula: crate::ScopedFormula) -> Box<foliage::Formula>
{
	match scoped_formula.free_variable_declarations.is_empty()
	{
		true => scoped_formula.formula,
		false => Box::new(foliage::Formula::for_all(scoped_formula.free_variable_declarations,
			scoped_formula.formula)),
	}
}
