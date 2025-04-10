import cli;
import std;

auto main(int argc, char** argv) -> int {
	auto root = cli::command{"pkg"};
	root.set_description("package manager");
	root.add_option("verbose",
	                cli::option{.description = "enable verbose output", .name = "verbose", .short_name = 'v'});
	root.add_option("config",
	                cli::option{
						.description = "specify config file",
						.name = "config",
						.short_name = 'c',
						.value = cli::value<std::string>("path"),
					});
	root.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});
	root.set_action([](const cli::command& cmd) -> std::optional<int> {
		if (cmd.got_option("help") || !cmd.got_command()) {
			cmd.print_help();
			return 0;
		}

		if (cmd.got_option("verbose")) {
			std::println("verbose set");
		}

		if (const auto value = cmd.option_value<std::string>("config")) {
			std::println("config file set to {}", *value);
		}

		return std::nullopt;
	});

	auto& install = root.add_command("install");
	install.set_usage("packages...");
	install.set_description("install packages");
	install.add_option("yes", cli::option{.description = "auto-confirm", .name = "yes", .short_name = 'y'});
	install.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});
	install.set_action([](const cli::command& cmd) {
		if (cmd.got_option("help")) {
			cmd.print_help();
			return 0;
		}

		if (cmd.got_option("yes")) {
			std::println("auto-confirmed");
		}

		std::println("installing packages:");

		for (auto arg : cmd.arguments()) {
			std::println("  - {}", arg);
		}
		return 0;
	});

	auto& remove = root.add_command("remove");
	remove.set_usage("packages...");
	remove.set_description("remove packages");
	remove.add_option("purge", cli::option{.description = "remove config files", .name = "purge", .short_name = 'p'});
	remove.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});
	remove.set_action([](const cli::command& cmd) {
		if (cmd.got_option("help")) {
			cmd.print_help();
			return 0;
		}

		if (cmd.got_option("purge")) {
			std::println("purge set");
		}

		std::println("removing packages:");
		for (auto arg : cmd.arguments()) {
			std::println("  - {}", arg);
		}

		return 0;
	});

	auto& search = root.add_command("search");
	search.set_usage("query");
	search.set_description("search packages");
	search.add_option("exact", cli::option{.description = "exact match only", .name = "exact", .short_name = 'e'});
	search.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});
	search.set_action([](const cli::command& cmd) {
		if (cmd.got_option("help")) {
			cmd.print_help();
			return 0;
		}

		if (cmd.arguments().size() != 1) {
			std::println("expected one argument for query");
			return 1;
		}

		if (cmd.got_option("exact")) {
			std::println("exact matches set");
		}

		std::println("search results for: {}", cmd.arguments()[0]);

		return 0;
	});

	auto& update = root.add_command("update");
	update.set_description("update package list");
	update.add_option("quiet", cli::option{.description = "suppress output", .name = "quiet", .short_name = 'q'});
	update.set_action([](const cli::command& cmd) {
		if (cmd.got_option("help")) {
			cmd.print_help();
			return 0;
		}

		if (!cmd.got_option("quiet")) {
			std::println("updating package database...");
		}

		std::println("database updated");

		return 0;
	});

	auto& list = root.add_command("list");
	list.set_description("list installed packages");
	list.add_option("all", cli::option{.description = "show all packages", .name = "all", .short_name = 'a'});
	list.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});
	list.set_action([](const cli::command& cmd) {
		if (cmd.got_option("help")) {
			cmd.print_help();
			return 0;
		}

		std::println("installed packages:");
		if (cmd.got_option("all")) {
			std::println("  - package1\n  - package2\n  - package3");
		} else {
			std::println("  - package1\n  - package2");
		}

		return 0;
	});

	const auto result = root.execute(argc, argv);
	if (!result) {
		std::println(std::cerr, "pkg: {}", result.error());
		return 1;
	}
	return *result;
}
