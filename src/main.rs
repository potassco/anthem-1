use structopt::StructOpt as _;

#[derive(Debug, structopt::StructOpt)]
#[structopt(name = "anthem", about = "Use first-order theorem provers with answer set programs.")]
enum Command
{
	#[structopt(about = "Verifies a logic program against a specification")]
	VerifyProgram
	{
		/// ASP input program (one or multiple files)
		#[structopt(parse(from_os_str), required(true))]
		input: Vec<std::path::PathBuf>,

		/// Output format (human-readable, tptp)
		#[structopt(long, default_value = "human-readable")]
		output_format: anthem::output::Format,

		/// Input predicates (examples: p/0, q/2)
		#[structopt(long, parse(try_from_str = anthem::parse_predicate_declaration))]
		input_predicates: Vec<std::rc::Rc<foliage::PredicateDeclaration>>,
	}
}

fn main()
{
	pretty_env_logger::init();

	let command = Command::from_args();

	match command
	{
		Command::VerifyProgram
		{
			input,
			output_format,
			input_predicates,
		}
			=>
		{
			if let Err(error) = anthem::translate::verify_properties::translate(&input,
				input_predicates.into_iter().collect::<foliage::PredicateDeclarations>(),
				output_format)
			{
				log::error!("could not translate input program: {}", error);
				std::process::exit(1)
			}
		},
	}
}
