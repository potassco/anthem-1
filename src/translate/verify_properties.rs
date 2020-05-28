mod head_type;
mod translate_body;

use head_type::*;
use translate_body::*;

use foliage::flavor::{PredicateDeclaration as _};

struct PredicateDefinitions
{
	pub parameters: std::rc::Rc<crate::VariableDeclarations>,
	pub definitions: Vec<crate::OpenFormula>,
}

type Definitions =
	std::collections::BTreeMap::<std::rc::Rc<crate::PredicateDeclaration>, PredicateDefinitions>;

pub(crate) struct Translator<'p>
{
	problem: &'p mut crate::Problem,
	definitions: Definitions,
}

struct Logger;

impl clingo::Logger for Logger
{
	fn log(&mut self, code: clingo::Warning, message: &str)
	{
		log::warn!("clingo warning ({:?}): {}", code, message);
	}
}

impl<'p> clingo::StatementHandler for Translator<'p>
{
	fn on_statement(&mut self, statement: &clingo::ast::Statement) -> bool
	{
		match statement.statement_type()
		{
			clingo::ast::StatementType::Rule(ref rule) =>
			{
				if let Err(error) = self.read_rule(rule)
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

impl<'p> Translator<'p>
{
	pub fn new(problem: &'p mut crate::Problem) -> Self
	{
		Self
		{
			problem,
			definitions: Definitions::new(),
		}
	}

	pub fn translate<P>(&mut self, program_path: P) -> Result<(), crate::Error>
	where
		P: AsRef<std::path::Path>,
	{
		// Read input program
		let program = std::fs::read_to_string(program_path.as_ref())
			.map_err(
				|error| crate::Error::new_read_file(program_path.as_ref().to_path_buf(), error))?;

		clingo::parse_program_with_logger(&program, self, &mut Logger, std::u32::MAX)
			.map_err(|error| crate::Error::new_translate(error))?;

		log::info!("read input program “{}”", program_path.as_ref().display());

		let completed_definition = |predicate_declaration, definitions: &mut Definitions|
		{
			match definitions.remove(predicate_declaration)
			{
				// This predicate symbol has at least one definition, so build the disjunction of those
				Some(predicate_definitions) =>
				{
					let or_arguments = predicate_definitions.definitions.into_iter()
						.map(|x| crate::existential_closure(x))
						.collect::<Vec<_>>();
					let or = crate::Formula::or(or_arguments);

					let head_arguments = predicate_definitions.parameters.iter()
						.map(|x| crate::Term::variable(std::rc::Rc::clone(x)))
						.collect::<Vec<_>>();

					let head_predicate = crate::Formula::predicate(
						std::rc::Rc::clone(predicate_declaration), head_arguments);

					let completed_definition =
						crate::Formula::if_and_only_if(vec![head_predicate, or]);

					let open_formula = crate::OpenFormula
					{
						free_variable_declarations: predicate_definitions.parameters,
						formula: completed_definition,
					};

					crate::universal_closure(open_formula)
				},
				// This predicate has no definitions, so universally falsify it
				None =>
				{
					let parameters = std::rc::Rc::new((0..predicate_declaration.arity()).map(
						|_| std::rc::Rc::new(crate::VariableDeclaration::new_generated(
							crate::Domain::Program)))
						.collect::<Vec<_>>());

					let head_arguments = parameters.iter()
						.map(|x| crate::Term::variable(std::rc::Rc::clone(x)))
						.collect();

					let head_predicate = crate::Formula::predicate(
						std::rc::Rc::clone(predicate_declaration), head_arguments);

					let not = crate::Formula::not(Box::new(head_predicate));

					let open_formula = crate::OpenFormula
					{
						free_variable_declarations: parameters,
						formula: not,
					};

					crate::universal_closure(open_formula)
				},
			}
		};

		for predicate_declaration in self.problem.predicate_declarations.borrow_mut().iter()
		{
			// Don’t perform completion for input predicates and built-in predicates
			if *predicate_declaration.is_input.borrow() || predicate_declaration.is_built_in()
			{
				continue;
			}

			let statement_kind = crate::problem::StatementKind::CompletedDefinition(
				std::rc::Rc::clone(&predicate_declaration));

			let completed_definition = completed_definition(predicate_declaration,
				&mut self.definitions);

			*predicate_declaration.dependencies.borrow_mut() =
				Some(crate::collect_predicate_declarations(&completed_definition));

			let statement_name =
				format!("completed_definition_{}", predicate_declaration.tptp_statement_name());

			let statement = crate::problem::Statement::new(statement_kind, completed_definition)
				.with_name(statement_name);

			self.problem.add_statement(crate::problem::SectionKind::CompletedDefinitions,
				statement);
		}

		Ok(())
	}

	fn read_rule(&mut self, rule: &clingo::ast::Rule)
		-> Result<(), crate::Error>
	{
		let head_type = determine_head_type(rule.head(), self.problem)?;

		match &head_type
		{
			HeadType::SingleAtom(head_atom)
			| HeadType::ChoiceWithSingleAtom(head_atom) =>
			{
				if !self.definitions.contains_key(&head_atom.predicate_declaration)
				{
					let parameters = std::rc::Rc::new((0..head_atom.predicate_declaration.arity())
						.map(
							|_| std::rc::Rc::new(crate::VariableDeclaration::new_generated(
								crate::Domain::Program)))
						.collect());

					self.definitions.insert(
						std::rc::Rc::clone(&head_atom.predicate_declaration),
						PredicateDefinitions
						{
							parameters,
							definitions: vec![],
						});
				}

				// TODO: refactor
				let predicate_definitions =
					self.definitions.get_mut(&head_atom.predicate_declaration).unwrap();

				let parameters = std::rc::Rc::clone(&predicate_definitions.parameters);
				let free_variable_declarations = std::cell::RefCell::new(vec![]);
				let free_layer =
					crate::VariableDeclarationStackLayer::Free(free_variable_declarations);
				let parameters_layer =
					crate::VariableDeclarationStackLayer::bound(&free_layer, parameters);

				let mut definition_arguments =
					translate_body(rule.body(), self.problem, &parameters_layer)?;

				// TODO: refactor
				assert_eq!(predicate_definitions.parameters.len(), head_atom.arguments.len());

				if let HeadType::ChoiceWithSingleAtom(_) = head_type
				{
					let head_arguments = predicate_definitions.parameters.iter()
						.map(|x| crate::Term::variable(std::rc::Rc::clone(x)))
						.collect::<Vec<_>>();

					let head_predicate = crate::Formula::predicate(
						std::rc::Rc::clone(&head_atom.predicate_declaration), head_arguments);

					definition_arguments.push(head_predicate);
				}

				let mut head_atom_arguments_iterator = head_atom.arguments.iter();

				for parameter in predicate_definitions.parameters.iter()
				{
					let head_atom_argument = head_atom_arguments_iterator.next().unwrap();

					let translated_head_term = crate::translate::common::choose_value_in_term(
						head_atom_argument, std::rc::Rc::clone(parameter), self.problem,
						&parameters_layer)?;

					definition_arguments.push(translated_head_term);
				}

				// TODO: refactor
				let free_variable_declarations = match free_layer
				{
					crate::VariableDeclarationStackLayer::Free(free_variable_declarations)
						=> free_variable_declarations.into_inner(),
					_ => unreachable!(),
				};

				let definition = match definition_arguments.len()
				{
					1 => definition_arguments.pop().unwrap(),
					0 => crate::Formula::true_(),
					_ => crate::Formula::and(definition_arguments),
				};

				let definition = crate::OpenFormula
				{
					free_variable_declarations: std::rc::Rc::new(free_variable_declarations),
					formula: definition,
				};

				predicate_definitions.definitions.push(definition);
			},
			HeadType::IntegrityConstraint =>
			{
				let free_variable_declarations = std::cell::RefCell::new(vec![]);
				let free_layer =
					crate::VariableDeclarationStackLayer::Free(free_variable_declarations);

				let mut arguments = translate_body(rule.body(), self.problem, &free_layer)?;

				// TODO: refactor
				let free_variable_declarations = match free_layer
				{
					crate::VariableDeclarationStackLayer::Free(free_variable_declarations)
						=> free_variable_declarations.into_inner(),
					_ => unreachable!(),
				};

				let formula = match arguments.len()
				{
					1 => crate::Formula::not(Box::new(arguments.pop().unwrap())),
					0 => crate::Formula::false_(),
					_ => crate::Formula::not(Box::new(crate::Formula::and(arguments))),
				};

				let open_formula = crate::OpenFormula
				{
					free_variable_declarations: std::rc::Rc::new(free_variable_declarations),
					formula,
				};

				let integrity_constraint = crate::universal_closure(open_formula);

				let statement = crate::problem::Statement::new(
					crate::problem::StatementKind::IntegrityConstraint, integrity_constraint)
					.with_name("integrity_constraint".to_string());

				self.problem.add_statement(crate::problem::SectionKind::IntegrityConstraints,
					statement);
			},
			HeadType::Trivial => log::info!("skipping trivial rule"),
		}

		Ok(())
	}
}
