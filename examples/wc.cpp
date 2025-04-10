import std;
import cli;

struct file_stats {
	std::string name;
	std::size_t lines;
	std::size_t words;
	std::size_t chars;
};

constexpr auto num_digits(std::size_t n) {
	auto digits = 1;
	while (n >= 10) {
		n /= 10;
		++digits;
	}
	return digits;
}

auto count_stats(std::istream& input) {
	auto lines = 0uz;
	auto words = 0uz;
	auto chars = 0uz;

	for (auto line = std::string{}; std::getline(input, line);) {
		++lines;
		chars += line.size() + 1;
		auto words_view = line | std::views::split(' ') | std::views::filter([](auto&& w) { return !w.empty(); });
		words += static_cast<std::size_t>(std::ranges::distance(words_view));
	}

	return std::tuple{lines, words, chars};
}

auto main(int argc, char** argv) -> int {
	auto cli = cli::command{"wc"};
	cli.set_usage("[file]...");

	cli.add_option("bytes", {.description = "print the byte counts", .name = "bytes", .short_name = 'c'});
	cli.add_option("lines", {.description = "print the newline counts", .name = "lines", .short_name = 'l'});
	cli.add_option("words", {.description = "print the word counts", .name = "words", .short_name = 'w'});
	cli.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});

	if (auto res = cli.parse(argc, argv); !res) {
		std::println(std::cerr, "wc: {}", res.error());
		return 1;
	}

	if (cli.got_option("help")) {
		cli.print_help();
		return 0;
	}

	const auto show_all = !(cli.got_option("lines") || cli.got_option("words") || cli.got_option("bytes"));

	const auto print_stats = [&](std::size_t lines, std::size_t words, std::size_t chars, std::int32_t width) {
		if (show_all || cli.got_option("lines")) std::print("{:>{}} ", lines, width);
		if (show_all || cli.got_option("words")) std::print("{:>{}} ", words, width);
		if (show_all || cli.got_option("bytes")) std::print("{:>{}} ", chars, width);
	};

	const auto args = cli.arguments();

	if (args.empty()) {
		const auto [lines, words, chars] = count_stats(std::cin);
		const auto width = num_digits(chars);
		print_stats(lines, words, chars, width);
		std::println();
		return 0;
	}

	if (args.size() == 1) {
		const auto arg = args[0];
		auto file = std::ifstream{std::string{arg}};
		if (!file) {
			std::println(std::cerr, "wc: {}: No such file", arg);
			return 1;
		}
		const auto [lines, words, chars] = count_stats(file);
		const auto width = num_digits(chars);
		print_stats(lines, words, chars, width);
		std::println("{}", arg);
		return 0;
	}

	auto had_error = false;
	auto stats = std::vector<file_stats>{};
	auto total_lines = 0uz;
	auto total_words = 0uz;
	auto total_chars = 0uz;

	for (const auto& arg : args) {
		auto file = std::ifstream{std::string{arg}};
		if (!file) {
			std::println(std::cerr, "wc: {}: No such file", arg);
			had_error = true;
			continue;
		}
		const auto [lines, words, chars] = count_stats(file);
		total_lines += lines;
		total_words += words;
		total_chars += chars;
		stats.emplace_back(std::string{arg}, lines, words, chars);
	}

	auto width = num_digits(total_chars);
	for (const auto& s : stats) {
		print_stats(s.lines, s.words, s.chars, width);
		std::println("{}", s.name);
	}
	print_stats(total_lines, total_words, total_chars, width);
	std::println("{}", "total");

	return had_error ? 1 : 0;
}
