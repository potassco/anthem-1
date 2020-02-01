mod context;
mod translate_body;
mod translate_head;

pub use context::Context;

use translate_body::translate_body;
use translate_head::determine_head_type;

pub fn read(rule: &clingo::ast::Rule, context: &mut Context) -> Result<(), crate::Error>
{
	let translated_body = translate_body(rule.body(), context)?;

	let head_type = determine_head_type(rule.head(),
		|name, arity| context.find_or_create_predicate_declaration(name, arity))?;

	match head_type
	{
		translate_head::HeadType::ChoiceWithSingleAtom(test) =>
			log::debug!("translating choice rule with single atom"),
		translate_head::HeadType::IntegrityConstraint =>
			log::debug!("translating integrity constraint"),
		translate_head::HeadType::Trivial =>
		{
			log::debug!("skipping trivial rule");
			return Ok(());
		},
		_ => (),
	}

	Ok(())
}
