import std;
import cli;

auto error(std::string_view msg) {
	std::println(std::cerr, "simple: {}", msg);
}

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"simple"};
	cli.set_description("Simple cli example");
	cli.set_arguments_validator(cli::exact_args(0));
	cli.add_option("value", {.description = "print value", .name = "value", .value = cli::value<double>{}});
	cli.add_option("help", cli::option{.description = "print help", .name = "help", .short_name = 'h'});
	cli.add_option("version", {.description = "print version", .name = "version", .short_name = 'v'});

	if (const auto res = cli.parse(argc, argv); !res) {
		error(res.error());
		return 1;
	}

	if (const auto help = cli.option_value<bool>("help")) {
		cli.print_help();
		return 0;
	}

	if (const auto value = cli.option_value<double>("value")) {
		std::println("{}", *value);
	}

	return 0;
}
