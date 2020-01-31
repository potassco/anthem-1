mod context;
mod translate_body;

pub use context::Context;
use translate_body::translate_body;

pub fn read(rule: &clingo::ast::Rule, context: &mut Context)
{
	println!("{:?}", translate_body(rule.body(), context));
}
