export module cli:command;

import :flag;
import std;

export namespace cli {

class command {
public:
	explicit command(std::string_view name) : name_{name} {}

	auto set_usage(std::string_view usage) -> void {
		usage_ = usage;
	}

	auto set_description(std::string_view description) -> void {
		description_ = description;
	}

	auto set_action(const std::function<void(const command&)>& action) {
		action_ = action;
	}

	auto add_flag(const std::string& name, const flag& flag) -> void {
		flags_.emplace_back(name, flag);
	}

	auto add_command(const std::string_view name) -> command& {
		children_.emplace_back(std::make_unique<command>(name));
		auto& child = children_.back();
		child->parent_ = *this;
		return *child;
	}

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
																											 return child->name_.size();
																										 }));

			help += std::format("Commands:\n");
			for (const auto& child : children_) {
				help += std::format("  {:{}}  {}\n", child->name_, commands_padding, child->description_);
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
	std::vector<std::pair<std::string, struct flag>> flags_;
	std::optional<std::reference_wrapper<const command>> parent_;
	std::vector<std::unique_ptr<command>> children_;
};

}  // namespace cli
