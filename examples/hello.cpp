import cli;
import std;

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"hello"};
	cli.add_option("name",
	               cli::option{
					   .description = "entity to greet",
					   .name = "name",
					   .short_name = 'n',
					   .value = cli::value<std::string>{"entity"},
				   });
	cli.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});

	if (const auto res = cli.parse(argc, argv); !res) {
		std::println(std::cerr, "echo: {}", res.error());
		return 1;
	}

	if (cli.got_option("help")) {
		cli.print_help();
		return 0;
	}

	if (const auto name = cli.option_value<std::string>("name")) {
		std::println("Hello, {}!", *name);
	} else {
		std::println("Hello, World!");
	}
}
