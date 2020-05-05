pub fn run<P>(program_path: P, specification_path: P, output_format: crate::output::Format)
where
	P: AsRef<std::path::Path>
{
	//let context = crate::translate::verify_properties::Context::new();
	let mut problem = crate::Problem::new();

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

	problem.prove(crate::ProofDirection::Both);

	log::info!("done");
}
