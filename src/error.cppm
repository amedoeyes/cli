export module cli:error;

import std;

export namespace cli {

enum class error : std::uint8_t {
	no_value,
	type_mismatch,
};

}  // namespace cli
