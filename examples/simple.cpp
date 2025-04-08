import std;
import cli;

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"simple"};
	cli.set_description("Simple cli example");
	cli.add_option("value",
	               {
					   .description = "print value",
					   .name = "value",
					   .value = cli::value<std::string>{},
				   });
	cli.add_option("help",
	               cli::option{
					   .description = "print help",
					   .name = "help",
					   .short_name = 'h',
				   });
	cli.add_option("version",
	               {
					   .description = "print version",
					   .name = "version",
					   .short_name = 'v',
				   });

	if (const auto res = cli.parse(argc, argv); !res) {
		std::println(std::cerr, "simple: {}", res.error());
		return 1;
	}

	if (const auto value = cli.option_value<bool>("help")) {
		cli.print_help();
		return 0;
	}

	if (const auto value = cli.option_value<bool>("version")) {
		std::println("1.0.0");
		return 0;
	}

	if (const auto value = cli.option_value<std::string>("value")) {
		std::println("{}", *value);
	}

	return 0;
}
