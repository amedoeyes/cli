export module cli:command;

import :flag;
import std;

export namespace cli {

class command {
public:
	explicit command(std::string_view name) : name_{name} {}

	auto usage(std::string_view usage) -> command& {
		usage_ = usage;
		return *this;
	}

	auto description(std::string_view description) -> command& {
		description_ = description;
		return *this;
	}

	auto subcommand(const command& cmd) -> command& {
		children_.emplace_back(cmd);
		children_.back().parent_ = *this;
		return *this;
	}

	auto flag(const std::string& name, const flag& flag) -> command& {
		flags_.emplace_back(name, flag);
		return *this;
	}

	auto print_help() const -> void {
		auto help = std::string{};

		if (!description_.empty()) help += std::format("{}\n\n", description_);

		auto parents = std::string{};
		auto parent = parent_;
		while (parent) {
			parents.insert(0, parent->get().name_ + " ");
			parent = parent->get().parent_;
		};

		help += std::format("Usage:\n  {}", parents);
		if (!usage_.empty()) help += usage_;
		else help += name_;
		help += "\n\n";

		if (!children_.empty()) {
			const auto commands_padding = std::ranges::max(children_ | std::views::transform([](auto&& child) {
																											 return child.name_.size();
																										 }));

			help += std::format("Commands:\n");
			for (const auto& child : children_) {
				help += std::format("  {:{}}  {}\n", child.name_, commands_padding, child.description_);
			}
			help += "\n";
		}

		if (!flags_.empty()) {
			auto flag_usage = [](const auto& flag) -> std::string {
				if (!flag.usage.empty()) return flag.usage;
				const auto short_flag = (flag.short_name != '\0') ? std::format("-{}", flag.short_name) : std::string{};
				const auto long_flag = (!flag.name.empty()) ? std::format("--{}", flag.name) : std::string{};
				return std::format("{:>2}, {}", short_flag, long_flag);
			};

			const auto flags_padding = std::ranges::max(flags_ | std::views::values | std::views::transform([&](auto&& flag) {
																										return flag_usage(flag).size();
																									}));

			help += std::format("Flags:\n", usage_);
			for (const auto& flag : std::views::values(flags_)) {
				help += std::format("  {:{}}", flag_usage(flag), flags_padding);
				if (!flag.description.empty()) help += std::format("  {}", flag.description);
				help += '\n';
			}
		}

		std::print("{}", help);
	}

private:
	std::string name_;
	std::string usage_;
	std::string description_;
	std::vector<std::pair<std::string, class flag>> flags_;
	std::optional<std::reference_wrapper<const command>> parent_;
	std::vector<command> children_;
};

}  // namespace cli
