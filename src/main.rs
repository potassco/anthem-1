struct StatementHandler<'context>
{
	context: &'context mut anthem::translate::verify_properties::Context,
}

impl clingo::StatementHandler for StatementHandler<'_>
{
	fn on_statement(&mut self, statement: &clingo::ast::Statement) -> bool
	{
		match statement.statement_type()
		{
			clingo::ast::StatementType::Rule(ref rule) =>
			{
				anthem::translate::verify_properties::read(rule, self.context)
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
	let mut context = anthem::translate::verify_properties::Context::new();
	let mut statement_handler = StatementHandler
	{
		context: &mut context
	};
	clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)?;

	Ok(())
}
