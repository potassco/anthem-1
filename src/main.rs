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

	if let Err(error) = anthem::translate::verify_properties::translate(&program)
	{
		log::error!("could not translate input program: {}", error);
		std::process::exit(1)
	}
}
