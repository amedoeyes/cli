export module cli:value;

import std;

export namespace cli {

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
                                   value<std::uint8_t>,
                                   value<std::int16_t>,
                                   value<std::uint16_t>,
                                   value<std::int32_t>,
                                   value<std::uint32_t>,
                                   value<std::int64_t>,
                                   value<std::uint64_t>,
                                   value<float>,
                                   value<double>,
                                   value<std::string>>;

}  // namespace cli
