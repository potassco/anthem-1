use structopt::StructOpt as _;

#[derive(Debug, structopt::StructOpt)]
#[structopt(name = "anthem", about = "Use first-order theorem provers with answer set programs.")]
struct Options
{
	#[structopt(parse(from_os_str))]
	input: Vec<std::path::PathBuf>,
}

fn main()
{
	pretty_env_logger::init();

	let options = Options::from_args();

	if let Err(error) = anthem::translate::verify_properties::translate(&options.input)
	{
		log::error!("could not translate input program: {}", error);
		std::process::exit(1)
	}
}
