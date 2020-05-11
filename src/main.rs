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

		/// Proof direction (forward, backward, both)
		#[structopt(long, default_value = "forward")]
		proof_direction: anthem::problem::ProofDirection,
	}
}

fn main()
{
	pretty_env_logger::init_custom_env("ANTHEM_LOG");

	let command = Command::from_args();

	match command
	{
		Command::VerifyProgram
		{
			program_path,
			specification_path,
			proof_direction,
		}
			=> anthem::commands::verify_program::run(&program_path, &specification_path,
				proof_direction),
	}
}
