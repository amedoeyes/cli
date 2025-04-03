export module cli;

import std;

export namespace cli {

template<typename T>
class value {
public:
	auto name(std::string_view name) -> value& {
		name_ = name;
		return *this;
	}

	auto default_value(T&& value) -> class value& {
		default_value_ = std::move(value);
		return *this;
	}

	[[nodiscard]]
	auto name() const -> std::string_view {
		return name_;
	}

	[[nodiscard]]
	auto default_value() const -> const std::optional<T>& {
		return default_value_;
	}

private:
	std::string name_;
	std::optional<T> default_value_;
};

using value_type = std::variant<value<bool>, value<std::int64_t>, value<double>, value<std::string>>;

class flag {
public:
	auto usage(std::string_view usage) -> flag& {
		usage_ = usage;
		return *this;
	}

	auto description(std::string_view description) -> flag& {
		description_ = description;
		return *this;
	}

	auto name(std::string_view name) -> flag& {
		name_ = name;
		return *this;
	}

	auto short_name(std::string_view name) -> flag& {
		short_name_ = name;
		return *this;
	}

	template<typename T>
	auto value(const value<T> value) -> flag& {
		value_ = value;
		return *this;
	}

	[[nodiscard]]
	auto usage() const -> std::string_view {
		return usage_;
	}

	[[nodiscard]]
	auto description() const -> std::string_view {
		return description_;
	}

	[[nodiscard]]
	auto name() const -> std::string_view {
		return name_;
	}

	[[nodiscard]]
	auto short_name() const -> std::string_view {
		return short_name_;
	}

	[[nodiscard]]
	auto value() const -> const std::optional<value_type>& {
		return value_;
	}

private:
	std::string usage_;
	std::string description_;
	std::string name_;
	std::string short_name_;
	std::optional<value_type> value_;
};

class command {
public:
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
		if (!description_.empty()) std::print("{}\n\n", description_);

		std::print("Usage:\n  {}\n\n", usage_);

		const auto commands_padding = std::ranges::max(children_ //
		                                               | std::views::transform([](auto&& child) {
																											 return child.usage_.size();
																										 }));

		std::print("Commands:\n", usage_);
		for (const auto& child : children_) {
			std::println("  {:{}}  {}", child.usage_, commands_padding, child.description_);
		}
		std::println("");

		const auto flags_padding = std::ranges::max(flags_ //
		                                            | std::views::values //
		                                            | std::views::transform([](auto&& flag) {
																										return flag.usage().size();
																									}));

		std::print("Flags:\n", usage_);
		for (const auto& flag : std::views::values(flags_)) {
			std::println("  {:{}}  {}", flag.usage(), flags_padding, flag.description());
		}
	}

private:
	std::string usage_;
	std::string description_;
	std::vector<std::pair<std::string, class flag>> flags_;
	std::optional<std::reference_wrapper<const command>> parent_;
	std::vector<command> children_;
};

}  // namespace cli
