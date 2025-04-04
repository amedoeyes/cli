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

		help += std::format("Usage:\n  {} {}\n\n", parents, usage_);

		const auto commands_padding = std::ranges::max(children_ //
		                                               | std::views::transform([](auto&& child) {
																											 return child.name_.size();
																										 }));

		help += std::format("Commands:\n");
		for (const auto& child : children_) {
			help += std::format("  {:{}}  {}\n", child.name_, commands_padding, child.description_);
		}
		help += "\n";

		const auto flags_padding = std::ranges::max(flags_ //
		                                            | std::views::values //
		                                            | std::views::transform([](auto&& flag) {
																										return flag.usage().size();
																									}));

		help += std::format("Flags:\n", usage_);
		for (const auto& flag : std::views::values(flags_)) {
			help += std::format("  {:{}}  {}\n", flag.usage(), flags_padding, flag.description());
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
