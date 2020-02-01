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
	head_atom_parameters: foliage::VariableDeclarations,
	definitions: Vec<ScopedFormula>,
}

pub fn read(rule: &clingo::ast::Rule) -> Result<(), crate::Error>
{
	let mut function_declarations = foliage::FunctionDeclarations::new();
	let mut predicate_declarations = foliage::PredicateDeclarations::new();
	let mut variable_declaration_stack = foliage::VariableDeclarationStack::new();

	let head_type = determine_head_type(rule.head(),
		|name, arity| super::common::find_or_create_predicate_declaration(
			&mut predicate_declarations, name, arity))?;

	let mut definitions
		= std::collections::BTreeMap::<std::rc::Rc<foliage::PredicateDeclaration>, Definitions>::new();

	let declare_predicate_parameters = |predicate_declaration: &foliage::PredicateDeclaration|
	{
		(0..predicate_declaration.arity)
			.map(|_| std::rc::Rc::new(foliage::VariableDeclaration
				{
					name: "<anonymous>".to_string(),
				}))
			.collect()
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

			let definitions = definitions.get(&head_atom.predicate_declaration).unwrap();
		},
		HeadType::ChoiceWithSingleAtom(test) =>
			log::debug!("translating choice rule with single atom"),
		HeadType::IntegrityConstraint =>
			log::debug!("translating integrity constraint"),
		HeadType::Trivial =>
		{
			log::debug!("skipping trivial rule");
			return Ok(());
		},
	}

	Ok(())
}
