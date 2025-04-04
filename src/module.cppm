export module cli;

import std;

export namespace cli {

enum class error : std::uint8_t {
	no_value,
	type_mismatch,
};

template<typename T>
concept ValueType = std::is_same_v<T, bool> //
                 || std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t> //
                 || std::is_same_v<T, std::int16_t> || std::is_same_v<T, std::uint16_t> //
                 || std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::int32_t>//
                 || std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t> //
                 || std::is_same_v<T, float> || std::is_same_v<T, double>  //
                 || std::is_same_v<T, std::string>;

template<ValueType T>
struct value {
	value() = default;

	explicit value(const T& data) : data{data} {}

	std::optional<T> data;
};

using value_variant = std::variant<value<bool>,
                                   value<std::int8_t>,
                                   value<std::int16_t>,
                                   value<std::int32_t>,
                                   value<std::int64_t>,
                                   value<std::uint8_t>,
                                   value<std::uint16_t>,
                                   value<std::uint32_t>,
                                   value<std::uint64_t>,
                                   value<float>,
                                   value<double>,
                                   value<std::string>>;

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

	template<ValueType T>
	auto value() -> flag& {
		value_ = cli::value<T>{};
		return *this;
	}

	template<ValueType T>
	auto value(const T& data) -> flag& {
		value_ = cli::value{data};
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

	template<ValueType T>
	[[nodiscard]]
	auto value() const -> std::expected<T, error> {
		if (!value_) return std::unexpected{error::no_value};
		const auto* value = std::get_if<cli::value<T>>(&*value_);
		if (!value) return std::unexpected{error::type_mismatch};
		return *value->data;
	}

private:
	std::string usage_;
	std::string description_;
	std::string name_;
	std::string short_name_;
	std::optional<value_variant> value_;
};

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
