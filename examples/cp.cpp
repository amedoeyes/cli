import std;
import cli;

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"cp"};
	cli.set_usage("src dest");
	cli.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});

	if (const auto res = cli.parse(argc, argv); !res) {
		std::println(std::cerr, "cp: {}", res.error());
		return 1;
	}

	if (cli.got_option("help")) {
		cli.print_help();
		return 0;
	}

	const auto args = cli.arguments();

	if (args.size() == 0) {
		std::println(std::cerr, "cp: expected src and dest but got nothing");
		return 1;
	}
	if (args.size() == 1) {
		std::println(std::cerr, "cp: expected dest but only got src");
		return 1;
	}
	if (args.size() > 2) {
		std::println(std::cerr, "cp: expected only src and dest but got more");
		return 1;
	}

	const auto src = args[0];
	const auto dest = args[1];

	auto ec = std::error_code{};
	std::filesystem::copy(src, dest, std::filesystem::copy_options::overwrite_existing, ec);
	if (ec) {
		std::println(std::cerr, "cp: {}: {}", src, ec.message());
		return 1;
	}
}
