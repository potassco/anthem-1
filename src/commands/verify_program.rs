pub fn run<P>(program_path: P, specification_path: P,
	proof_direction: crate::problem::ProofDirection, no_simplify: bool,
	color_choice: crate::output::ColorChoice)
where
	P: AsRef<std::path::Path>,
{
	//let context = crate::translate::verify_properties::Context::new();
	let mut problem = crate::Problem::new(color_choice);

	log::info!("reading specification “{}”", specification_path.as_ref().display());

	let specification_content = match std::fs::read_to_string(specification_path.as_ref())
	{
		Ok(specification_content) => specification_content,
		Err(error) =>
		{
			log::error!("could not access specification file: {}", error);
			std::process::exit(1)
		},
	};

	// TODO: rename to read_specification
	if let Err(error) = crate::input::parse_specification(&specification_content, &mut problem)
	{
		log::error!("could not read specification: {}", error);
		std::process::exit(1)
	}

	log::info!("read specification “{}”", specification_path.as_ref().display());

	log::info!("reading input program “{}”", program_path.as_ref().display());

	// TODO: make consistent with specification call (path vs. content)
	if let Err(error) = crate::translate::verify_properties::Translator::new(&mut problem)
		.translate(program_path)
	{
		log::error!("could not translate input program: {}", error);
		std::process::exit(1)
	}

	if let Err(error) = problem.check_consistency(proof_direction)
	{
		log::error!("{}", error);
		std::process::exit(1)
	}

	if !no_simplify
	{
		if let Err(error) = problem.simplify()
		{
			log::error!("could not simplify translated program: {}", error);
			std::process::exit(1)
		}
	}

	if let Err(error) = problem.prove(proof_direction)
	{
		log::error!("could not verify program: {}", error);
		std::process::exit(1)
	}
}
