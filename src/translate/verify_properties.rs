mod context;
mod translate_body;
mod translate_head;

pub use context::Context;

use translate_body::translate_body;
use translate_head::determine_head_type;

pub fn read(rule: &clingo::ast::Rule, context: &mut Context) -> Result<(), crate::Error>
{
	let test = translate_body(rule.body(), context)?;

	println!("{:?}", test);

	let test = determine_head_type(rule.head(),
		|name, arity| context.find_or_create_predicate_declaration(name, arity))?;

	match test
	{
		translate_head::HeadType::ChoiceWithSingleAtom(_) => println!("choice single"),
		translate_head::HeadType::IntegrityConstraint => println!("integrity"),
		translate_head::HeadType::Trivial => println!("trivial"),
		_ => (),
	}

	Ok(())
}
