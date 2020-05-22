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
	match crate::input::parse_specification(&specification_content, &mut problem)
	{
		Ok(_) => (),
		Err(error) =>
		{
			log::error!("could not read specification: {}", error);
			std::process::exit(1)
		}
	}

	log::info!("read specification “{}”", specification_path.as_ref().display());

	log::info!("reading input program “{}”", program_path.as_ref().display());

	// TODO: make consistent with specification call (path vs. content)
	match crate::translate::verify_properties::Translator::new(&mut problem).translate(program_path)
	{
		Ok(_) => (),
		Err(error) =>
		{
			log::error!("could not translate input program: {}", error);
			std::process::exit(1)
		}
	}

	match problem.check_that_only_input_and_output_predicates_are_used()
	{
		Ok(_) => (),
		Err(error) => log::warn!("{}", error),
	}

	match problem.restrict_to_output_predicates()
	{
		Ok(_) => (),
		Err(error) =>
		{
			log::error!("could not restrict problem to output predicates: {}", error);
			std::process::exit(1)
		}
	}

	if !no_simplify
	{
		match problem.simplify()
		{
			Ok(_) => (),
			Err(error) =>
			{
				log::error!("could not simplify translated program: {}", error);
				std::process::exit(1)
			}
		}
	}

	match problem.prove(proof_direction)
	{
		Ok(()) => (),
		Err(error) =>
		{
			log::error!("could not verify program: {}", error);
			std::process::exit(1)
		}
	}

	log::info!("done");
}
