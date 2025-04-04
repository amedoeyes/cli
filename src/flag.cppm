export module cli;

import :value;
import std;

export namespace cli {

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
	auto value() const -> std::expected<T, std::string> {
		if (!value_) return std::unexpected{"flag does not have a value"};
		const auto* value = std::get_if<cli::value<T>>(&*value_);
		if (!value) return std::unexpected{"type mismatch"};
		return *value->data;
	}

private:
	std::string usage_;
	std::string description_;
	std::string name_;
	std::string short_name_;
	std::optional<value_variant> value_;
};

}  // namespace cli
