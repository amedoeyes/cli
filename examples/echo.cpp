import std;
import cli;

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"echo"};
	cli.set_usage("[string]...");
	cli.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});

	if (const auto res = cli.parse(argc, argv); !res) {
		std::println(std::cerr, "echo: {}", res.error());
		return 1;
	}

	if (cli.got_option("help")) {
		cli.print_help();
		return 0;
	}

	const auto args = cli.arguments();
	for (const auto i : std::views::iota(0uz, args.size())) {
		std::print("{}", args[i]);
		if (i != args.size() - 1) std::print(" ");
	}
	std::println();
}
