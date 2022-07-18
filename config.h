#pragma once
#include <vector>
#include <string>
#include <initializer_list>
#include <string_view>

using namespace std::literals;
using sv = std::string_view;
using strvec = std::initializer_list<sv>;

// args to pass into dmenu -I and -n are required
constexpr static strvec DMENU_ARGS = { "-I"sv, "-n"sv, "-i"sv, "-c"sv, "-l"sv, "20"sv };

// the shell that will run the exec commands
constexpr static sv SHELL = "sh";
// the args to pass to the shell, the desktop entry's exec command will be appended
constexpr static strvec SHELL_ARGS = { "-c"sv };

// the terminal that will open terminal programs
constexpr static sv TERMINAL = "kitty";
// the args to pass to the terminal, the desktop entry's exec command will be appended
constexpr static strvec TERMINAL_ARGS = { "sh"sv, "-c"sv };
