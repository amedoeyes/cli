# CLI

Simple modern C++23 command-line interface library.

## Example

```c++
import cli;
import std;

auto main(int argc, char** argv) -> int {
    auto cli = cli::command{"hello"};
    cli.add_option("name",
                   {
                       .description = "entity to greet",
                       .name = "name",
                       .short_name = 'n',
                       .value = cli::value<std::string>{"entity"},
                   });
    cli.add_option("help", {.description = "display this help and exit", .name = "help", .short_name = 'h'});

    if (const auto res = cli.parse(argc, argv); !res) {
        std::println(std::cerr, "hello: {}", res.error());
        return 1;
    }

    if (cli.got_option("help")) {
        cli.print_help();
        return 0;
    }

    if (const auto name = cli.option_value<std::string>("name")) {
        std::println("Hello, {}!", *name);
    } else {
        std::println("Hello, World!");
    }
}
```

## Usage

### Command

To create a command for a program with no sub-commands it's just:

```c++
auto cli = command{"echo"};
```

Otherwise with sub-commands you'll need to add them with `add_command` method:

```c++
auto root = command{"pkg"};
auto& sub = root.add_command("install");
```

### Command Meta-data

```c++
cli.set_usage("[file]...");
cli.set_description("Concatenate files");
```

### Command Action

A command action is just a lambda that takes its command as a parameter and returns `std::optional<int>`. If it returned a `std::nullopt` then command chain execution will continue, otherwise if it returned an `int` the chain is broken with that int as the exit status.

```c++
root.set_action([](const cli::command& cmd) -> std::optional<int> {
    if (!cmd.got_command()) {
        cmd.print_help();
        return 0;
    }
    return std::nullopt;
});

install.set_action([](const cli::command& cmd) {
    if (cmd.got_option("help")) {
        cmd.print_help();
        return 0;
    }
    std::println("installing packages:");
    for (auto arg : cmd.arguments()) {
        std::println("  - {}", arg);
    }
    return 0;
});
```

### Command Help

Each command has a `print_help` method that prints a generated help to stdout. Example of a generated help:

```
package manager

Usage: pkg [options] command

Commands:
  install  install packages
  remove   remove packages
  search   search packages
  update   update package list
  list     list installed packages

Options:
  -v, --verbose      enable verbose output
  -c, --config path  specify config file
  -h, --help         display this help and exit
```

### Command Options

To add options to a command use the `add_option` method and pass it a name which will be used as an identifier and an `option` struct. `add_option` expects the option to have either a name or a short name and short name must be alpha numeric.

To define the value of an option set the `value` field of the `option` struct with a `value` struct. The `value` struct can take a name which will be used for the usage of the option and a default value.

> [!NOTE]
> If `usage` field is set then it'll be used instead of the value in the help message.

```c++
cli.add_option("config", {
    .usage = "path",
    .description = "Config file path",
    .name = "config",
    .short_name = 'c',
    .value = cli::value<std::string>{"name", "config.ini"}
});

cli.add_option("verbose", {
    .description = "Enable verbose output",
    .short_name = 'v'
});
```

### Grouping

To group options and commands, first you'll need an enum to hold group IDs. In the help message groups appear in ascending order based on their enum values.

```c++
enum class option_groups { config, meta };
enum class command_groups { common, query };
```

Next you'll need to register each group and its name like so:

```c++
cli.set_option_groups({
    {groups::config, "Config"},
    {groups::meta, "Meta"},
});

root.set_command_groups({
    {groups::common, "Common"},
    {groups::query, "Query"},
});
```

And then add options and commands with the group ID like this:

```c++
cli.add_option("verbose", option_groups::meta, { /* ... */ });
cli.add_option("config", option_groups::config, { /* ... */ });

auto& install = root.add_command("install", command_groups::common);
auto& search = root.add_command("search", command_groups::query);
```

### Using Option

After parsing, options can be extracted using the `option_value` method like this:

```c++
if (const auto path = cli.option_value<std::string>("config")) {
    // ...
}
```

Or if you don't care about the actual value and want to know if an option was used you can use the `got_option` like this:

```c++
if (cli.got_option("help")) {
    // ...
}
```

### Acessing Arguments

To access parsed arguments use the `arguments` method.

```c++
for (auto arg : cli.arguments()) {
    std::println("{}", arg);
}
```

### Parsing And Executing

You can use the `parse` method to start parsing arguments based on option and command definitions like this:

```c++
if (const auto res = cli.parse(argc, argv); !res) {
    std::println(std::cerr, "{}", res.error());
    return 1;
}
```

If you have sub-commands use the `execute` method to parse and run the command chain. `execute` will return the exit code of a command or 0.

```c++
const auto result = root.execute(argc, argv);
if (!result) {
    std::println(std::cerr, "{}", result.error());
    return 1;
}
return *result;
```

## Contributing

Contributions are welcome! If you notice a bug or want to add a feature, feel free to open an issue or submit a pull request.
