module;

#include <cassert>

export module cli;

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

using primitive = std::variant<bool,
                               std::int8_t,
                               std::uint8_t,
                               std::int16_t,
                               std::uint16_t,
                               std::int32_t,
                               std::uint32_t,
                               std::int64_t,
                               std::uint64_t,
                               float,
                               double,
                               std::string>;

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

template<typename T>
struct get_value_type;

template<class... Ts>
struct overloads : Ts... {
	using Ts::operator()...;
};

template<typename T>
struct get_value_type<cli::value<T>> {
	using type = T;
};

template<typename Variant, typename... Funcs>
	requires requires(Variant&& variant, Funcs&&... funcs) {
				 std::visit(overloads{std::forward<Funcs>(funcs)...}, std::forward<Variant>(variant));
			 }
auto visit(Variant&& variant, Funcs&&... funcs) {
	return std::visit(overloads{std::forward<Funcs>(funcs)...}, std::forward<Variant>(variant));
}

template<typename T>
auto parse_number(std::string_view str, std::int32_t base = 10) -> std::expected<T, std::errc> {
	auto value = T{};
	const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value, base);
	if (ec != std::errc{}) return std::unexpected{ec};
	if (ptr != str.data() + str.size()) return std::unexpected{std::errc::invalid_argument};
	return value;
}

export namespace cli {

struct option {
	std::string usage;
	std::string description;
	std::string name;
	char short_name{'\0'};
	std::optional<value_variant> value;
};

}  // namespace cli

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

	auto add_option(const std::string& name, const option& option) -> void {
		assert((!option.name.empty() || option.short_name != '\0') && "name or short_name must be set");
		assert(option.short_name == '\0'
		       || std::isalnum(static_cast<unsigned char>(option.short_name)) && "short_name must be alphanumeric");
		options_.emplace_back(name, option);
	}

	auto add_command(const std::string_view name) -> command& {
		children_.emplace_back(std::make_unique<command>(name));
		auto& child = children_.back();
		child->parent_ = *this;
		return *child;
	}

	[[nodiscard]]
	auto arguments() const -> std::span<const std::string_view> {
		return arguments_;
	}

	template<ValueType T>
	[[nodiscard]]
	auto option_value(std::string_view name) const -> std::optional<T> {
		if (const auto it = option_values_.find(std::string{name}); it != option_values_.end()) {
			if (const auto* value = std::get_if<T>(&it->second)) return *value;
		}
		return std::nullopt;
	}

	auto print_help() const -> void {
		auto help = std::string{};

		if (!description_.empty()) help += std::format("{}\n\n", description_);

		help += "Usage: ";
		if (!usage_.empty()) {
			help += usage_;
		} else {
			auto parents = std::string{};
			auto parent = parent_;
			while (parent) {
				parents.insert(0, parent->get().name_ + " ");
				parent = parent->get().parent_;
			};
			help += parents + name_;
			if (!options_.empty()) help += " [options]";
			if (!children_.empty()) help += " [command]";
		}
		help += "\n\n";

		if (!children_.empty()) {
			const auto commands_padding = std::ranges::max(children_ | std::views::transform([](auto&& child) {
															   return child->name_.size();
														   }));

			help += "Commands:\n";
			for (const auto& child : children_) {
				help += std::format("  {:{}}  {}\n", child->name_, commands_padding, child->description_);
			}
			help += "\n";
		}

		if (!options_.empty()) {
			auto option_usage = [](const auto& option) -> std::string {
				if (!option.usage.empty()) return option.usage;
				auto usage = std::string{};

				const auto name = (!option.name.empty()) ? std::format("--{}", option.name) : std::string{};
				const auto short_name = (option.short_name != '\0') ? std::format("-{}", option.short_name)
				                                                    : std::string{};

				if (!name.empty() && short_name.empty()) usage += std::format("    {}", name);
				else if (name.empty() && !short_name.empty()) usage += std::format("{}", short_name);
				else usage += std::format("{}, {}", short_name, name);

				if (option.value) {
					const auto type = visit(
						*option.value,
						[](const value<bool>&) { return "bool"; },
						[](const value<std::int8_t>&) { return "s8"; },
						[](const value<std::uint8_t>&) { return "u8"; },
						[](const value<std::int16_t>&) { return "s16"; },
						[](const value<std::uint16_t>&) { return "u16"; },
						[](const value<std::int32_t>&) { return "s32"; },
						[](const value<std::uint32_t>&) { return "u32"; },
						[](const value<std::int64_t>&) { return "s64"; },
						[](const value<std::uint64_t>&) { return "u64"; },
						[](const value<float>&) { return "f32"; },
						[](const value<double>&) { return "f64"; },
						[](const value<std::string>&) { return "str"; });

					const auto default_value = visit(*option.value,
					                                 [](auto&& v) -> std::optional<primitive> { return v.data; });
					const auto name = visit(*option.value,
					                        [](auto&& v) -> std::optional<std::string> { return v.name; });
					usage += " ";
					if (default_value) usage += "[";
					if (name) usage += *name;
					else usage += type;
					if (default_value) usage += "]";
				}

				return usage;
			};

			const auto options_padding = std::ranges::max(options_ | std::views::values
			                                              | std::views::transform([&](auto&& option) {
																return option_usage(option).size();
															}));

			help += "Options:\n";
			for (const auto& option : std::views::values(options_)) {
				help += std::format("  {:{}}", option_usage(option), options_padding);
				if (!option.description.empty()) help += std::format("  {}", option.description);
				help += '\n';
			}
		}

		std::print("{}", help);
	}

	auto parse(std::int32_t argc, char** argv) -> std::expected<void, std::string> {
		const auto args = std::span(std::next(argv), argc - 1)
		                | std::views::transform([](auto&& a) { return std::string_view(a); })
		                | std::ranges::to<std::vector>();
		if (const auto res = parse(args); !res) return std::unexpected{res.error()};
		return {};
	}

	auto execute(std::int32_t argc, char** argv) -> std::expected<void, std::string> {
		const auto args = std::span(std::next(argv), argc - 1)
		                | std::views::transform([](auto&& a) { return std::string_view(a); })
		                | std::ranges::to<std::vector>();
		auto cmds = parse(args);
		if (!cmds) return std::unexpected{cmds.error()};
		for (const auto& cmd : *cmds) cmd.get().action_(cmd.get());
		return {};
	}

private:
	std::string name_;
	std::string usage_;
	std::string description_;
	std::function<void(const command&)> action_;
	std::vector<std::string_view> arguments_;
	std::vector<std::pair<std::string, option>> options_;
	std::unordered_map<std::string, primitive> option_values_;
	std::optional<std::reference_wrapper<const command>> parent_;
	std::vector<std::unique_ptr<command>> children_;

	auto parse(std::span<const std::string_view> args)
		-> std::expected<std::list<std::reference_wrapper<const command>>, std::string> {
		auto index = 0;
		const auto curr = [&] { return args[index]; };
		const auto at_end = [&] { return index == args.size(); };
		const auto next = [&] {
			if (index < args.size()) ++index;
		};
		auto force_positional = false;

		while (!at_end()) {
			if (!force_positional && curr() == "--") {
				force_positional = true;
				next();
			}

			if (!force_positional && curr().starts_with('-')) {
				auto option_name = std::string_view{};
				auto option_value = std::optional<std::string_view>{};
				const auto option_prefix = std::string_view{curr().starts_with("--") ? "--" : "-"};
				const auto option_error = [&](const std::string_view msg) {
					return std::unexpected{std::format("'{}{}' {}", option_prefix, option_name, msg)};
				};

				if (option_prefix == "--") {
					const auto eq_pos = curr().find('=');
					if (eq_pos != std::string::npos) {
						option_name = curr().substr(2, eq_pos - 2);
						option_value = curr().substr(eq_pos + 1);
					} else {
						option_name = curr().substr(2);
					}
				} else if (option_prefix == "-" && curr().size() == 2) {
					option_name = curr().substr(1, 1);
					if (curr().size() > 2) {
						if (curr()[2] == '=') option_value = curr().substr(3);
						else option_value = curr().substr(2);
					}
				} else {
					return option_error("is not a valid option");
				}
				next();

				const auto it = std::ranges::find_if(options_, [&](auto&& p) {
					return option_prefix == "--" ? p.second.name == option_name : p.second.short_name == option_name[0];
				});
				if (it == options_.end()) return option_error("is not a valid option");
				const auto& [key, option] = *it;

				if (!option.value && option_value)
					return option_error(std::format("does not expect a value but got '{}'", *option_value));

				if (option.value) {
					const auto default_value = visit(*option.value,
					                                 [](auto&& v) -> std::optional<primitive> { return v.data; });
					if (default_value) option_values_[key] = *default_value;

					if (!option_value && !at_end() && !curr().starts_with("-")) {
						option_value = curr();
						next();
					} else if (!default_value && !option_value) {
						return option_error("expects a value");
					}

					if (option_value) {
						const auto value = visit(
							*option.value,

							[&](const cli::value<bool>&) -> std::expected<primitive, std::string> {
								if (*option_value == "true") return true;
								if (*option_value == "false") return false;
								return option_error(std::format(
									"expects a boolean value of either 'true' or 'false' " "but " "got '{}'",
									*option_value));
							},

							[&](const cli::value<std::string>&) -> std::expected<primitive, std::string> {
								return std::string{*option_value};
							},

							[&](const auto& type) -> std::expected<primitive, std::string> {
								using T = typename get_value_type<std::decay_t<decltype(type)>>::type;

								if constexpr (std::is_integral_v<T>) {
									const auto value = parse_number<T>(*option_value);
									if (!value) {
										return option_error(std::format("expects an integer number but got '{}'",
									                                    *option_value));
									}
									return *value;
								} else if constexpr (std::is_floating_point_v<T>) {  // TODO: replace with parse_number
								                                                     // when std::from_chars support
								                                                     // floats
									try {
										if constexpr (std::is_same_v<T, float>) {
											return std::stof(std::string{*option_value});
										} else if constexpr (std::is_same_v<T, double>) {
											return std::stod(std::string{*option_value});
										}
									} catch (const std::exception&) {
										return option_error(std::format("expects a floating-point number but got '{}'",
									                                    *option_value));
									}
								}
							});

						if (!value) return std::unexpected{value.error()};
						option_values_[key] = *value;
					}
				} else {
					option_values_[key] = true;
				}

				continue;
			}

			if (!force_positional) {
				const auto it = std::ranges::find_if(children_, [&](auto&& c) { return c->name_ == curr(); });
				if (it != children_.end()) {
					next();
					auto cmds = (*it)->parse(args.subspan(index));
					if (!cmds) return std::unexpected{cmds.error()};
					cmds->emplace_front(*this);
					return *cmds;
				}
			}

			if (!at_end()) {
				arguments_.emplace_back(curr());
				next();
			}
		}

		return std::list{std::cref(*this)};
	}
};

}  // namespace cli
