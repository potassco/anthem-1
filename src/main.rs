struct Context
{
}

struct StatementHandler<'context>
{
	context: &'context mut Context,
}

impl clingo::StatementHandler for StatementHandler<'_>
{
	fn on_statement(&mut self, statement: &clingo::ast::Statement) -> bool
	{
		match statement.statement_type()
		{
			clingo::ast::StatementType::Rule(ref rule) =>
			{
				println!("got rule {:?}", rule)
			},
			_ => println!("got other kind of statement"),
		}

		true
	}
}

struct Logger;

impl clingo::Logger for Logger
{
	fn log(&mut self, code: clingo::Warning, message: &str)
	{
		println!("clingo warning ({:?}): {}", code, message);
	}
}

fn main() -> Result<(), Box<dyn std::error::Error>>
{
	let program = std::fs::read_to_string("test.lp")?;
	let mut context = Context{};
	let mut statement_handler = StatementHandler{context: &mut context};
	clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)?;

	Ok(())
}
