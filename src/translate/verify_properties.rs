mod head_type;
mod translate_body;

use head_type::*;
use translate_body::*;

pub struct ScopedFormula
{
	free_variable_declarations: foliage::VariableDeclarations,
	formula: foliage::Formula,
}

pub struct Definitions
{
	head_atom_parameters: std::rc::Rc<foliage::VariableDeclarations>,
	definitions: Vec<ScopedFormula>,
}

pub fn read(rule: &clingo::ast::Rule) -> Result<(), crate::Error>
{
	use super::common::FindOrCreatePredicateDeclaration;

	let mut function_declarations = foliage::FunctionDeclarations::new();
	let mut predicate_declarations = foliage::PredicateDeclarations::new();
	let mut variable_declaration_stack = foliage::VariableDeclarationStack::new();

	let head_type = determine_head_type(rule.head(),
		|name, arity| predicate_declarations.find_or_create(name, arity))?;

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

			variable_declaration_stack.push(std::rc::Rc::clone(&definitions.head_atom_parameters));

			let mut definition_arguments = translate_body(rule.body(), &mut function_declarations,
				&mut predicate_declarations, &mut variable_declaration_stack)?;

			assert_eq!(definitions.head_atom_parameters.len(), head_atom.arguments.len());

			let mut head_atom_arguments_iterator = head_atom.arguments.iter();

			for head_atom_parameter in definitions.head_atom_parameters.iter()
			{
				let head_atom_argument = head_atom_arguments_iterator.next().unwrap();

				let translated_head_term = crate::translate::common::choose_value_in_term(
					head_atom_argument, head_atom_parameter, &mut function_declarations,
					&mut variable_declaration_stack)?;

				definition_arguments.push(Box::new(translated_head_term));
			}

			variable_declaration_stack.pop();

			let definition = foliage::Formula::And(definition_arguments);

			let definition = ScopedFormula
			{
				free_variable_declarations: variable_declaration_stack.free_variable_declarations,
				formula: definition,
			};

			log::trace!("translated formula: {:?}", definition.formula);

			definitions.definitions.push(definition);
		},
		HeadType::ChoiceWithSingleAtom(_) =>
			log::debug!("translating choice rule with single atom"),
		HeadType::IntegrityConstraint =>
			log::debug!("translating integrity constraint"),
		HeadType::Trivial => log::debug!("skipping trivial rule"),
	}

	Ok(())
}
