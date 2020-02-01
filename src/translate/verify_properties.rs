mod context;
mod head_type;
mod translate_body;

pub use context::Context;

use head_type::*;
use translate_body::*;

pub fn read(rule: &clingo::ast::Rule, context: &mut Context) -> Result<(), crate::Error>
{
	let translated_body = translate_body(rule.body(), context)?;

	let head_type = determine_head_type(rule.head(),
		|name, arity| context.find_or_create_predicate_declaration(name, arity))?;

	match head_type
	{
		HeadType::ChoiceWithSingleAtom(test) =>
			log::debug!("translating choice rule with single atom"),
		HeadType::IntegrityConstraint =>
			log::debug!("translating integrity constraint"),
		HeadType::Trivial =>
		{
			log::debug!("skipping trivial rule");
			return Ok(());
		},
		_ => (),
	}

	Ok(())
}
