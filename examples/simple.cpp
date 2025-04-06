import std;
import cli;

auto error(std::string_view msg) {
	std::println(std::cerr, "simple: {}", msg);
}

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"simple"};
	cli.set_usage("simple [command] [flag]...");
	cli.set_description("Simple cli example");
	cli.add_flag("value",
	             {
					 .usage = "  , --value VALUE",
					 .description = "print value",
					 .name = "value",
					 .value = cli::value<std::string>{},
				 });
	cli.add_flag("help", cli::flag{.description = "print help", .name = "help", .short_name = 'h'});
	cli.add_flag("version", {.description = "print version", .name = "version", .short_name = 'v'});

	if (const auto res = cli.parse(argc, argv); !res) {
		error(res.error());
		return 1;
	}

	const auto help = cli.flag_value<bool>("help");
	if (help) {
		cli.print_help();
		return 0;
	}

	const auto value = cli.flag_value<std::string>("value");
	if (value) std::println("{}", *value);

	std::println("{}", cli.arguments());

	return 0;
}
