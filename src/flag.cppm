export module cli:flag;

import :value;
import std;

export namespace cli {

struct flag {
	std::string usage;
	std::string description;
	std::string name;
	char short_name{'\0'};
	std::optional<value_variant> value;
};

}  // namespace cli
