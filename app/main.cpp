#include <iostream>

#include <boost/program_options.hpp>

int main(int argc, char **argv)
{
	namespace po = boost::program_options;

	po::options_description description("Allowed options");
	description.add_options()
		("help,h", "display this help message")
		("version,v", "Display version information.")
		("input,i", po::value<std::vector<std::string>>(), "Specify the PDDL or SAS input file.");

	po::positional_options_description positionalOptionsDescription;
	positionalOptionsDescription.add("input", -1);

	po::variables_map variablesMap;

	const auto printHelp =
		[&]()
		{
			std::cout << "Usage: anthem [files] [options]" << std::endl;
			std::cout << "Translate ASP programs to the language of first-order theorem provers." << std::endl << std::endl;

			std::cout << description;
		};

	try
	{
		po::store(po::command_line_parser(argc, argv)
			.options(description)
			.positional(positionalOptionsDescription)
			.run(),
			variablesMap);
		po::notify(variablesMap);
	}
	catch (const po::error &e)
	{
		std::cerr << e.what() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	if (variablesMap.count("help"))
	{
		printHelp();
		return EXIT_SUCCESS;
	}

	if (variablesMap.count("version"))
	{
		std::cout << "anthem version 0.1.0-git" << std::endl;
		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}
