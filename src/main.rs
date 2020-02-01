struct StatementHandler;

impl clingo::StatementHandler for StatementHandler
{
	fn on_statement(&mut self, statement: &clingo::ast::Statement) -> bool
	{
		match statement.statement_type()
		{
			clingo::ast::StatementType::Rule(ref rule) =>
			{
				if let Err(error) = anthem::translate::verify_properties::read(rule)
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

fn main()
{
	pretty_env_logger::init();

	let program = match std::fs::read_to_string("test.lp")
	{
		Ok(value) => value,
		Err(error) =>
		{
			log::error!("could not read input program: {}", error);
			std::process::exit(1);
		},
	};

	let mut statement_handler = StatementHandler{};

	match clingo::parse_program_with_logger(&program, &mut statement_handler, &mut Logger, std::u32::MAX)
	{
		Ok(()) => (),
		Err(error) =>
		{
			log::error!("could not translate input program: {}", error);
			std::process::exit(1);
		},
	}
}
