import std;
import cli;

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"cat"};
	cli.set_usage("[files]...");
	cli.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});

	if (const auto res = cli.parse(argc, argv); !res) {
		std::println(std::cerr, "cat: {}", res.error());
		return 1;
	}

	if (cli.option_value<bool>("help")) {
		cli.print_help();
		return 0;
	}

	auto had_error = false;

	if (cli.arguments().empty()) {
		std::print("{}", (std::stringstream{} << std::cin.rdbuf()).str());
	} else {
		for (const auto& arg : cli.arguments()) {
			auto file = std::ifstream{std::string{arg}};
			if (!file) {
				std::println(std::cerr, "cat: {}: No such file or directory", arg);
				had_error = true;
				continue;
			}
			std::print("{}", (std::stringstream{} << file.rdbuf()).str());
		}
	}

	return had_error ? 1 : 0;
}
