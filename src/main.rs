use structopt::StructOpt as _;

#[derive(Debug, structopt::StructOpt)]
#[structopt(name = "anthem", about = "Use first-order theorem provers with answer set programs.")]
enum Command
{
	#[structopt(about = "Verifies a logic program against a specification")]
	#[structopt(aliases = &["vprog"])]
	VerifyProgram
	{
		/// ASP input program file path
		#[structopt(name = "program", parse(from_os_str), required(true))]
		program_path: std::path::PathBuf,

		#[structopt(name = "specification", parse(from_os_str), required(true))]
		/// Specification file path
		specification_path: std::path::PathBuf,

		/// Output format (human-readable, tptp)
		#[structopt(long, default_value = "human-readable")]
		output_format: anthem::output::Format,
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
			program_path,
			specification_path,
			output_format,
		}
			=> anthem::commands::verify_specification::run(&program_path, &specification_path,
				output_format),
	}
}
