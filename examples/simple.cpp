import std;
import cli;

auto main() -> int {
	cli::command{}
		.usage("simple [command] [flag]...")
		.description("Simple cli example")
		.flag("help", cli::flag{}.usage("-h, --help").description("print help").name("help").short_name("h"))
		.flag("help", cli::flag{}.usage("-v, --version").description("print version").name("version").short_name("v"))
		.subcommand(cli::command{}.usage("sub1").description("subcommand 1"))
		.subcommand(cli::command{}.usage("sub2").description("subcommand 2"))
		.print_help();
}
