import std;
import cli;

auto main() -> int {
	cli::command{"simple"}
		.usage("simple [command] [flag]...")
		.description("Simple cli example")
		.flag("help", cli::flag{}.usage("-h, --help").description("print help").name("help").short_name("h"))
		.flag("help", cli::flag{}.usage("-v, --version").description("print version").name("version").short_name("v"))
		.subcommand(cli::command{"sub1"}.usage("sub1 [flag]...").description("subcommand 1"))
		.subcommand(cli::command{"sub2"}.usage("sub2 [flag]...").description("subcommand 2"))
		.print_help();
}
