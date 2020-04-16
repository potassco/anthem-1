use structopt::StructOpt as _;

#[derive(Debug, structopt::StructOpt)]
#[structopt(name = "anthem", about = "Use first-order theorem provers with answer set programs.")]
enum Command
{
	#[structopt(about = "Verifies a logic program against a specification")]
	#[structopt(aliases = &["verify-specification", "verify-spec", "vspec"])]
	VerifyProgram
	{
		/// ASP input program (one or multiple files)
		#[structopt(parse(from_os_str), required(true))]
		input: Vec<std::path::PathBuf>,

		/// Output format (human-readable, tptp)
		#[structopt(long, default_value = "human-readable")]
		output_format: anthem::output::Format,

		/// Input predicates (examples: p, q/2)
		#[structopt(long, parse(try_from_str = anthem::parse_predicate_declaration))]
		input_predicates: Vec<std::rc::Rc<foliage::PredicateDeclaration>>,

		/// Input constants (example: c, integer(n))
		#[structopt(long, parse(try_from_str = anthem::parse_constant_declaration))]
		input_constants: Vec<
			(std::rc::Rc<foliage::FunctionDeclaration>, anthem::Domain)>,
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
			input_constants,
		}
			=>
		{
			if let Err(error) = anthem::translate::verify_properties::translate(&input,
				input_predicates.into_iter().collect::<foliage::PredicateDeclarations>(),
				input_constants.into_iter().collect::<std::collections::BTreeMap<_, _>>(),
				output_format)
			{
				log::error!("could not translate input program: {}", error);
				std::process::exit(1)
			}
		},
	}
}
