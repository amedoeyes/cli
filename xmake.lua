set_project("cli")
set_version("1.0.0")
set_languages("cxx23")
set_toolchains("clang")
set_runtimes("c++_shared")

add_cxxflags(
	"-Weverything",
	"-Wno-c++98-compat",
	"-Wno-c++98-compat-pedantic",
	"-Wno-exit-time-destructors",
	"-Wno-missing-prototypes",
	"-Wno-padded",
	"-Wno-reserved-identifier", --doesn't work correctly with modules
	"-Wno-shadow-field-in-constructor",
	"-Wno-switch-default",
	"-Wno-unsafe-buffer-usage-in-container",
	"-Wno-weak-vtables"
)

add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.check", "mode.profile")
add_rules("plugin.compile_commands.autoupdate", { lsp = "clangd", outputdir = "build" })

set_policy("build.c++.modules", true)
target("cli", function()
	set_kind("static")
	add_files("src/**.cppm", { public = true })
end)

option("examples", {
	description = "Build examples",
	showmenu = true,
	default = false,
	type = "boolean",
})

if has_config("examples") then
	for _, example in ipairs(os.files("examples/*.cpp")) do
		target(path.basename(example), { files = example, deps = "cli" })
	end
end
