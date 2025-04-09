module;

#include <cassert>

export module cli;

import std;

namespace cli {

template<class... Ts>
struct overloads : Ts... {
	using Ts::operator()...;
};

template<typename Variant, typename... Funcs>
	requires requires(Variant&& variant, Funcs&&... funcs) {
				 std::visit(overloads{std::forward<Funcs>(funcs)...}, std::forward<Variant>(variant));
			 }
constexpr auto visit(Variant&& variant, Funcs&&... funcs) {
	return std::visit(overloads{std::forward<Funcs>(funcs)...}, std::forward<Variant>(variant));
}

template<typename T>
constexpr auto parse_number(std::string_view str, std::int32_t base = 10) noexcept
	-> std::expected<T, std::error_code> {
	auto value = T{};
	const auto* begin = str.data();
	const auto* end = std::next(str.data(), static_cast<std::int64_t>(str.size()));
	const auto [ptr, ec] = std::from_chars(begin, end, value, base);
	if (ec != std::errc{}) return std::unexpected{std::make_error_code(ec)};
	if (ptr != end) return std::unexpected{std::make_error_code(std::errc::invalid_argument)};
	return value;
}

}  // namespace cli

export namespace cli {

template<typename T>
concept Primitive = std::is_same_v<T, bool> //
                 || std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t> //
                 || std::is_same_v<T, std::int16_t> || std::is_same_v<T, std::uint16_t> //
                 || std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::int32_t>//
                 || std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t> //
                 || std::is_same_v<T, float> || std::is_same_v<T, double>  //
                 || std::is_same_v<T, std::string>;

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

template<Primitive T>
struct value {
	using type = T;
	std::optional<std::string> name;
	std::optional<T> data;
};

using primitive_value = std::variant<value<bool>,
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

struct option {
	std::string usage{};
	std::string description{};
	std::string name{};
	char short_name{'\0'};
	std::optional<primitive_value> value{std::nullopt};
};

struct group {
	std::int32_t id;
	std::string name;

	template<typename T>
		requires std::is_enum_v<T>
	group(T id, std::string_view name) noexcept : id{std::to_underlying(id)},
												  name{name} {}
};

class command {
public:
	explicit command(std::string_view name) noexcept : name_{name} {}

	auto set_usage(std::string_view usage) noexcept -> void {
		usage_ = usage;
	}

	auto set_description(std::string_view description) noexcept -> void {
		description_ = description;
	}

	auto set_action(const std::function<void(const command&)>& action) noexcept {
		action_ = action;
	}

	auto add_option(const std::string& name, const option& option) noexcept -> void {
		assert((!option.name.empty() || option.short_name != '\0') && "name or short_name must be set");
		assert(option.short_name == '\0'
		       || std::isalnum(static_cast<unsigned char>(option.short_name)) && "short_name must be alphanumeric");
		options_.emplace_back(name, std::nullopt, option);
	}

	template<typename T>
		requires std::is_enum_v<T>
	auto add_option(const std::string& name, T group, const option& option) noexcept -> void {
		assert((!option.name.empty() || option.short_name != '\0') && "name or short_name must be set");
		assert(option.short_name == '\0'
		       || std::isalnum(static_cast<unsigned char>(option.short_name)) && "short_name must be alphanumeric");
		options_.emplace_back(name, std::to_underlying(group), option);
	}

	auto set_option_groups(const std::vector<group>& groups) noexcept -> void {
		option_groups_ = groups;
	}

	auto add_command(std::string_view name) noexcept -> command& {
		commands_.emplace_back(std::make_unique<command>(name));
		auto& cmd = commands_.back();
		cmd->parent_ = *this;
		return *cmd;
	}

	template<typename T>
		requires std::is_enum_v<T>
	auto add_command(std::string_view name, T group) noexcept -> command& {
		commands_.emplace_back(std::make_unique<command>(name));
		auto& cmd = commands_.back();
		cmd->parent_ = *this;
		cmd->group_ = std::to_underlying(group);
		return *cmd;
	}

	auto set_command_groups(const std::vector<group>& groups) noexcept -> void {
		command_groups_ = groups;
	}

	[[nodiscard]]
	auto arguments() const noexcept -> std::span<const std::string_view> {
		return arguments_;
	}

	template<Primitive T>
	[[nodiscard]]
	auto option_value(std::string_view name) const noexcept -> std::optional<T> {
		if (const auto it = option_values_.find(std::string{name}); it != option_values_.end()) {
			if (const auto* value = std::get_if<T>(&it->second)) return *value;
		}
		return std::nullopt;
	}

	auto print_help() const noexcept -> void {
		auto help = std::string{};

		if (!description_.empty()) help += std::format("{}\n\n", description_);

		help += "Usage: ";
		auto parents = std::string{};
		auto parent = parent_;
		while (parent) {
			parents.insert(0, parent->get().name_ + " ");
			parent = parent->get().parent_;
		}
		help += parents + name_;
		if (!options_.empty()) help += " [options]";
		if (!usage_.empty()) help += std::format(" {}", usage_);
		else if (!commands_.empty()) help += " command";

		help += "\n\n";

		if (!commands_.empty()) {
			const auto cmd_pad = std::ranges::max(commands_ | std::views::transform([](auto&& cmd) {
													  return cmd->name_.size();
												  }));
			const auto cmd_help = [&](const command& cmd) -> std::string {
				return std::format("{:{}}  {}", cmd.name_, cmd_pad, cmd.description_);
			};

			help += "Commands:\n";
			if (command_groups_) {
				auto map = std::map<std::int32_t, std::vector<std::string>>{};
				for (const auto& cmd : commands_) {
					if (cmd->group_) map[*cmd->group_].emplace_back(cmd_help(*cmd));
					else map[std::numeric_limits<std::int32_t>::max()].emplace_back(cmd_help(*cmd));
				}

				for (const auto& [grp, msgs] : map) {
					if (grp == std::numeric_limits<std::int32_t>::max()) {
						help += "  Ungrouped:\n";
					} else {
						const auto it = std::ranges::find_if(*option_groups_, [&](auto&& g) { return g.id == grp; });
						assert(it != option_groups_->end() && "group not registered");
						help += std::format("  {}:\n", it->name);
					}

					for (const auto& msg : msgs) help += std::format("    {}\n", msg);
					help += '\n';
				}
			} else {
				for (const auto& cmd : commands_) help += std::format("  {}\n", cmd_help(*cmd));
				help += "\n";
			}
		}

		if (!options_.empty()) {
			auto opt_useage = [](const option& opt) -> std::string {
				auto usage = std::string{};

				const auto name = (!opt.name.empty()) ? std::format("--{}", opt.name) : std::string{};
				const auto short_name = (opt.short_name != '\0') ? std::format("-{}", opt.short_name) : std::string{};

				if (!name.empty() && short_name.empty()) usage += std::format("    {}", name);
				else if (name.empty() && !short_name.empty()) usage += std::format("{}", short_name);
				else usage += std::format("{}, {}", short_name, name);

				if (!opt.usage.empty()) {
					usage += opt.usage;
				} else if (opt.value) {
					const auto* type = visit(
						*opt.value,
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

					const auto default_value = visit(*opt.value,
					                                 [](auto&& v) -> std::optional<primitive> { return v.data; });
					const auto value_name = visit(*opt.value,
					                              [](auto&& v) -> std::optional<std::string> { return v.name; });
					usage += " ";
					if (default_value) usage += "[";
					if (value_name) usage += *value_name;
					else usage += type;
					if (default_value) usage += "]";
				}

				return usage;
			};

			const auto opt_pad = std::ranges::max(options_ | std::views::transform([&](auto&& opt) {
													  return opt_useage(std::get<2>(opt)).size();
												  }));
			const auto opt_help = [&](const option& opt) -> std::string {
				auto msg = std::string{};
				msg += std::format("{:{}}", opt_useage(opt), opt_pad);
				if (!opt.description.empty()) msg += std::format("  {}", opt.description);
				return msg;
			};

			help += "Options:\n";
			if (option_groups_) {
				auto map = std::map<std::int32_t, std::vector<std::string>>{};
				for (const auto& [_, grp, opt] : options_) {
					if (grp) map[*grp].emplace_back(opt_help(opt));
					else map[std::numeric_limits<std::int32_t>::max()].emplace_back(opt_help(opt));
				}

				for (const auto& [grp, msgs] : map) {
					if (grp == std::numeric_limits<std::int32_t>::max()) {
						help += "  Ungrouped:\n";
					} else {
						const auto it = std::ranges::find_if(*option_groups_, [&](auto&& g) { return g.id == grp; });
						assert(it != option_groups_->end() && "group not registered");
						help += std::format("  {}:\n", it->name);
					}

					for (const auto& msg : msgs) help += std::format("    {}\n", msg);
					help += '\n';
				}
			} else {
				for (const auto& [_, grp, opt] : options_) help += std::format("  {}\n", opt_help(opt));
				help += '\n';
			}
		}

		std::print("{}", help);
	}

	[[nodiscard]]
	auto parse(std::int32_t argc, char** argv) noexcept -> std::expected<void, std::string> {
		auto args = std::vector<std::string_view>{};
		args.reserve(static_cast<std::size_t>(argc));
		for (const auto i : std::views::iota(1z, static_cast<std::int64_t>(argc))) {
			args.emplace_back(*std::next(argv, i));
		}
		if (const auto res = parse(args); !res) return std::unexpected{res.error()};
		return {};
	}

	[[nodiscard]]
	auto execute(std::int32_t argc, char** argv) noexcept -> std::expected<void, std::string> {
		auto args = std::vector<std::string_view>{};
		args.reserve(static_cast<std::size_t>(argc));
		for (const auto i : std::views::iota(1z, static_cast<std::int64_t>(argc))) {
			args.emplace_back(*std::next(argv, i));
		}
		auto cmds = parse(args);
		if (!cmds) return std::unexpected{cmds.error()};
		for (const auto& cmd : *cmds) cmd.get().action_(cmd.get());
		return {};
	}

private:
	std::string name_;
	std::optional<std::int32_t> group_;
	std::string usage_;
	std::string description_;
	std::function<void(const command&)> action_;
	std::vector<std::string_view> arguments_;
	std::vector<std::tuple<std::string, std::optional<std::int32_t>, option>> options_;
	std::unordered_map<std::string, primitive> option_values_;
	std::optional<std::vector<group>> option_groups_;
	std::optional<std::reference_wrapper<const command>> parent_;
	std::vector<std::unique_ptr<command>> commands_;
	std::optional<std::vector<group>> command_groups_;

	auto parse(std::span<std::string_view> arguments) noexcept
		-> std::expected<std::list<std::reference_wrapper<const command>>, std::string> {
		auto index = 0uz;
		const auto curr = [&] { return arguments[index]; };
		const auto at_end = [&] { return index == arguments.size(); };
		const auto next = [&] {
			if (index < arguments.size()) ++index;
		};
		const auto extract = [&] {
			const auto arg = curr();
			next();
			return arg;
		};

		auto force_positional = false;

		while (!at_end()) {
			if (!force_positional && curr() == "--") {
				force_positional = true;
				next();
			}

			if (!force_positional && curr().starts_with('-') && curr() != "-") {
				auto option_name = std::string_view{};
				auto option_value = std::optional<std::string_view>{};
				const auto option_prefix = std::string_view{curr().starts_with("--") ? "--" : "-"};
				const auto option_error = [&]<typename... Args>(std::format_string<Args...> fmt, Args&&... args) {
					return std::unexpected{
						std::format("'{}{}' {}",
					                option_prefix,
					                option_name,
					                std::format(std::move(fmt), std::forward<Args>(args)...)),
					};
				};

				if (option_prefix == "--") {
					const auto eq_pos = curr().find('=');
					if (eq_pos != std::string::npos) {
						option_name = curr().substr(2, eq_pos - 2);
						option_value = curr().substr(eq_pos + 1);
					} else {
						option_name = curr().substr(2);
					}
				} else if (option_prefix == "-") {
					option_name = curr().substr(1, 1);
					if (curr().size() > 2) {
						if (curr()[2] == '=') option_value = curr().substr(3);
						else option_value = curr().substr(2);
					}
				} else {
					return option_error("is not a valid option");
				}
				next();

				const auto it = std::ranges::find_if(options_, [&](auto&& e) {
					return option_prefix == "--" ? std::get<2>(e).name == option_name
					                             : std::get<2>(e).short_name == option_name[0];
				});
				if (it == options_.end()) return option_error("is not a valid option");
				const auto& [name, _, option] = *it;

				if (!option.value && option_value) {
					return option_error("does not expect a value but got '{}'", *option_value);
				}

				if (option.value) {
					const auto default_value = visit(*option.value,
					                                 [](auto&& v) -> std::optional<primitive> { return v.data; });
					if (default_value) option_values_[name] = *default_value;

					if (!option_value && !at_end() && !curr().starts_with("-")) option_value = extract();
					else if (!default_value && !option_value) return option_error("expects a value");

					if (option_value) {
						const auto value = visit(*option.value, [&](auto&& v) -> std::expected<primitive, std::string> {
							using T = typename std::remove_cvref_t<decltype(v)>::type;
							const auto& str = *option_value;

							if constexpr (std::is_same_v<T, bool>) {
								if (str == "true") return true;
								if (str == "false") return false;
								return option_error("expects a boolean value of either 'true' or 'false' but got '{}'",
								                    str);
							} else if constexpr (std::is_same_v<T, std::string>) {
								return std::string{str};
							} else if constexpr (std::is_integral_v<T>) {
								if (const auto res = parse_number<T>(str)) return *res;
								return option_error("expects an integer number but got '{}'", str);
							} else if constexpr (std::is_floating_point_v<T>) {
								try {
									if constexpr (std::is_same_v<T, float>) return std::stof(std::string{str});
									else if constexpr (std::is_same_v<T, double>) return std::stod(std::string{str});
								} catch (const std::exception&) {
									return option_error("expects a floating-point number but got '{}'", str);
								}
							}
						});

						if (!value) return std::unexpected{value.error()};
						option_values_[name] = *value;
					}
				} else {
					option_values_[name] = true;
				}

				continue;
			}

			if (!force_positional) {
				const auto it = std::ranges::find_if(commands_, [&](auto&& c) { return c->name_ == curr(); });
				if (it != commands_.end()) {
					next();
					auto cmds = (*it)->parse(arguments.subspan(index));
					if (!cmds) return std::unexpected{cmds.error()};
					cmds->emplace_front(*this);
					return *cmds;
				}
			}

			if (!at_end()) arguments_.emplace_back(extract());
		}

		return std::list{std::cref(*this)};
	}
};

}  // namespace cli
