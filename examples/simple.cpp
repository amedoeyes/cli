import std;
import cli;

enum class groups : std::int8_t { group1, group2 };

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"simple"};
	cli.set_description("Simple cli example");
	cli.set_option_groups({
		{groups::group1, "Group 1"},
		{groups::group2, "Group 2"},
	});
	cli.add_option("value",
	               groups::group1,
	               {
					   .description = "print value",
					   .name = "value",
					   .value = cli::value<std::string>{},
				   });
	cli.add_option("help", groups::group2, {.description = "print help", .name = "help", .short_name = 'h'});
	cli.add_option("version", groups::group2, {.description = "print version", .name = "version", .short_name = 'v'});

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
