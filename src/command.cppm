export module cli:command;

import :flag;
import :value;
import std;

template<class... Ts>
struct overloads : Ts... {
	using Ts::operator()...;
};

template<typename Variant, typename... Funcs>
	requires requires(Variant&& variant, Funcs&&... funcs) {
				 std::visit(overloads{std::forward<Funcs>(funcs)...}, std::forward<Variant>(variant));
			 }
auto visit(Variant&& variant, Funcs&&... funcs) {
	return std::visit(overloads{std::forward<Funcs>(funcs)...}, std::forward<Variant>(variant));
}

export namespace cli {

struct arguments_validator {
	using validator_func = std::function<std::pair<bool, std::string>(std::span<const std::string_view>)>;

	template<typename F>
		requires std::invocable<F, std::span<const std::string_view>>
	          && std::same_as<std::invoke_result_t<F, std::span<const std::string_view>>, std::pair<bool, std::string>>
	arguments_validator(F&& f) : function_{std::forward<validator_func>(f)} {}

	auto operator()(std::span<const std::string_view> args) const -> std::pair<bool, std::string> {
		return function_(args);
	}

	constexpr auto operator+(const arguments_validator& rhs) const -> arguments_validator {
		const auto& left_func = function_;
		const auto& right_func = rhs.function_;
		return [left_func, right_func](std::span<const std::string_view> args) -> std::pair<bool, std::string> {
			if (const auto [ok, msg] = left_func(args); !ok) return std::pair{false, msg};
			return right_func(args);
		};
	}

private:
	validator_func function_;
};

constexpr auto exact_args(std::int32_t n, std::string_view msg = "") -> arguments_validator {
	return [=](std::span<const std::string_view> args) -> std::pair<bool, std::string> {
		if (n == args.size()) return {true, ""};
		return {
			false,
			msg.empty() ? std::format("expected {} argument(s) but got {}", n, args.size()) : std::string{msg},
		};
	};
}

constexpr auto min_args(std::int32_t n, std::string_view msg = "") -> arguments_validator {
	return [=](std::span<const std::string_view> args) -> std::pair<bool, std::string> {
		if (n <= args.size()) return {true, ""};
		return {
			false,
			msg.empty() ? std::format("expected at least {} argument(s) but got {}", n, args.size()) : std::string{msg},
		};
	};
}

constexpr auto max_args(std::int32_t n, std::string_view msg = "") -> arguments_validator {
	return [=](std::span<const std::string_view> args) -> std::pair<bool, std::string> {
		if (n >= args.size()) return {true, ""};
		return {
			false,
			msg.empty() ? std::format("expected at most {} argument(s) but got {}", n, args.size()) : std::string{msg},
		};
	};
}

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

	auto set_arguments_validator(const arguments_validator& validator) {
		arguments_validator_ = validator;
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

	[[nodiscard]]
	auto arguments() const -> std::span<const std::string_view> {
		return arguments_;
	}

	template<ValueType T>
	[[nodiscard]]
	auto flag_value(std::string_view name) const -> std::optional<T> {
		if (const auto it = flag_values_.find(std::string{name}); it != flag_values_.end()) {
			if (const auto* value = std::get_if<T>(&it->second)) return *value;
		}
		return std::nullopt;
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

			const auto flags_padding = std::ranges::max(flags_ | std::views::values
			                                            | std::views::transform([&](auto&& flag) {
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

	auto parse(std::int32_t argc, char** argv) -> std::expected<void, std::string> {
		const auto args = std::span(std::next(argv), argc - 1)
		                | std::views::transform([](auto&& a) { return std::string_view(a); })
		                | std::ranges::to<std::vector>();
		try {
			if (const auto res = parse(args); !res) return std::unexpected{res.error()};
		} catch (std::runtime_error& e) {
			return std::unexpected{e.what()};
		}
		return {};
	}

	auto execute(std::int32_t argc, char** argv) -> std::expected<void, std::string> {
		const auto args = std::span(std::next(argv), argc - 1)
		                | std::views::transform([](auto&& a) { return std::string_view(a); })
		                | std::ranges::to<std::vector>();
		auto cmds = parse(args);

		if (!cmds) return std::unexpected{cmds.error()};
		try {
			for (const auto& cmd : *cmds) cmd.get().action_(cmd.get());
		} catch (std::runtime_error& e) {
			return std::unexpected{e.what()};
		}
		return {};
	}

private:
	std::string name_;
	std::string usage_;
	std::string description_;
	std::function<void(const command&)> action_;
	std::vector<std::string_view> arguments_;
	std::optional<arguments_validator> arguments_validator_;
	std::vector<std::pair<std::string, flag>> flags_;
	std::unordered_map<std::string, primitive> flag_values_;
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

		while (!at_end()) {
			if (curr().starts_with('-')) {
				auto flag_name = std::string_view{};
				auto flag_value = std::optional<std::string_view>{};
				const auto flag_prefix = std::string_view{curr().starts_with("--") ? "--" : "-"};
				const auto error = [&](const std::string_view msg) {
					return std::unexpected{std::format("'{}{}' {}", flag_prefix, flag_name, msg)};
				};

				if (flag_prefix == "--") {
					const auto eq_pos = curr().find('=');
					if (eq_pos != std::string::npos) {
						flag_name = curr().substr(2, eq_pos - 2);
						flag_value = curr().substr(eq_pos + 1);
					} else {
						flag_name = curr().substr(2);
					}
				} else {
					flag_name = curr().substr(1, 1);
					if (curr().size() > 2) {
						if (curr()[2] == '=') flag_value = curr().substr(3);
						else flag_value = curr().substr(2);
					}
				}
				next();

				const auto it = std::ranges::find_if(flags_, [&](auto&& p) {
					return flag_prefix == "--" ? p.second.name == flag_name : p.second.short_name == flag_name[0];
				});
				if (it == flags_.end()) return error("is not a valid flag");
				const auto& [key, flag] = *it;

				if (!flag.value && flag_value)
					return error(std::format("does not expect a value but got '{}'", *flag_value));

				if (flag.value) {
					const auto default_value = visit(*flag.value,
					                                 [](auto&& v) -> std::optional<primitive> { return v.data; });
					if (default_value) flag_values_[key] = *default_value;

					if (!flag_value && !at_end() && !curr().starts_with("-")) {
						flag_value = curr();
						next();
					} else if (!default_value) {
						return error("expects a value");
					}

					if (flag_value) {
						const auto value = visit(
							*flag.value,

							[&](const std::optional<cli::value<bool>>&) -> std::expected<primitive, std::string> {
								if (*flag_value == "true") return true;
								if (*flag_value == "false") return false;
								return std::unexpected{std::format(
									"'{}{}' expects a boolean value of either 'true' or " "'false' but got " "'{}'",
									flag_prefix,
									flag_name,
									*flag_value)};
							},

							[&](const std::optional<cli::value<std::int64_t>>&)
								-> std::expected<primitive, std::string> {
								auto value = 0l;
								auto [_, ec] = std::from_chars(flag_value->data(),
							                                   std::next(flag_value->data(), long(flag_value->size())),
							                                   value);
								if (ec != std::errc{}) {
									return std::unexpected{std::format("'{}{}' expects an integer nubmer but got '{}'",
								                                       flag_prefix,
								                                       flag_name,
								                                       *flag_value)};
								}
								return value;
							},

							[&](const std::optional<cli::value<double>>&) -> std::expected<primitive, std::string> {
								auto value = 0.0;
								try {
									value = std::stod(std::string{*flag_value});
								} catch (...) {
									throw std::runtime_error{std::format("'{}{}' expects a dcimal number got but '{}'",
								                                         flag_prefix,
								                                         flag_name,
								                                         *flag_value)};
								}
								// // not supported by libc++ yet :/
                              // auto [_, ec] = std::from_chars(flag_value->data(),
							    //                                 std::next(flag_value->data(),
							    //                                 long(flag_value->size())), value);
							    // if (ec != std::errc{}) {
							    // 	return std::unexpected{
							    // 		std::format("'{}{}' expects a decimal nubmer but got '{}'", flag_prefix,
							    // flag_name, *flag_value)};
							    // }
								return value;
							},

							[&](const std::optional<cli::value<std::string>>&)
								-> std::expected<primitive, std::string> { return std::string{*flag_value}; });

						if (!value) return std::unexpected{value.error()};
						flag_values_[key] = *value;
					}
				} else {
					flag_values_[key] = true;
				}

				continue;
			}

			const auto it = std::ranges::find_if(children_, [&](auto&& c) { return c->name_ == curr(); });
			if (it != children_.end()) {
				next();

				if (arguments_validator_) {
					const auto [ok, msg] = (*arguments_validator_)(arguments_);
					if (!ok) return std::unexpected{msg};
				}

				auto cmds = (*it)->parse(args.subspan(index));
				if (!cmds) return std::unexpected{cmds.error()};
				cmds->emplace_front(*this);
				return *cmds;
			}

			if (!at_end()) {
				arguments_.emplace_back(curr());
				next();
			}
		}

		if (arguments_validator_) {
			const auto [ok, msg] = (*arguments_validator_)(arguments_);
			if (!ok) return std::unexpected{msg};
		}

		return std::list{std::cref(*this)};
	}
};

}  // namespace cli
