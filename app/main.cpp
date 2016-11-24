#include <iostream>

#include <boost/program_options.hpp>

#include <anthem/Context.h>
#include <anthem/Translation.h>

int main(int argc, char **argv)
{
	anthem::Context context;

	namespace po = boost::program_options;

	po::options_description description("Allowed options");
	description.add_options()
		("help,h", "Display this help message")
		("version,v", "Display version information")
		("input,i", po::value<std::vector<std::string>>(), "Input files");

	po::positional_options_description positionalOptionsDescription;
	positionalOptionsDescription.add("input", -1);

	po::variables_map variablesMap;

	const auto printHelp =
		[&]()
		{
			std::cout
				<< "Usage: anthem [files] [options]" << std::endl
				<< "Translate ASP programs to the language of first-order theorem provers." << std::endl << std::endl
				<< description;
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
		context.logger.log(anthem::output::Priority::Error, e.what());
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

	try
	{
		if (variablesMap.count("input"))
		{
			const auto &inputFiles = variablesMap["input"].as<std::vector<std::string>>();
			anthem::translate(inputFiles, context);
		}
		else
			anthem::translate("std::cin", std::cin, context);
	}
	catch (const std::exception &e)
	{
		context.logger.log(anthem::output::Priority::Error, e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
