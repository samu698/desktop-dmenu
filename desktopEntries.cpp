#include "desktopEntries.hpp"
#include <algorithm>
#include <iterator>
#include <optional>
#include <iterator>
#include <utility>

#include "iniParse.hpp"
#include "utils.hpp"

// https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

// ==========================================
// DesktopEntry
// ==========================================

std::string DesktopEntry::pathToId(const fs::path& base, const fs::path& path) {
	fs::path relative = path.lexically_relative(base);
	std::string id;
	auto length = std::distance(::begin(relative), ::end(relative));
	auto last = std::for_each_n(::begin(relative), length - 1, [&id](const auto& p) {
		id += p.native() + '-';
	});
	id += *last;
	return id;
}
DesktopEntry::DesktopEntry(std::string_view id) : id(id) {}
DesktopEntry::DesktopEntry(const fs::path& base, const fs::path& path) : path(path), id(pathToId(base, path)) {
	iniFile desktopFile(path.native());
	auto section = std::find(::begin(desktopFile), ::end(desktopFile), "Desktop Entry"sv);
	for (const auto& [ ename, value ] : section->entries) {
		if (ename == "Name") name = value;
		else if (ename == "Icon") icon = value;
		else if (ename == "Exec") exec = value;
		else if (ename == "Terminal") useTerminal = value == "true";
		else if (ename == "NoDisplay") hidden |= value == "true";
		else if (ename == "Hidden") hidden |= value == "true";
	}
}

std::string_view DesktopEntry::getId() const { return id; }
const fs::path& DesktopEntry::getPath() const { return path; }
std::string_view DesktopEntry::getName() const { return name; }
std::string_view DesktopEntry::getExec() const { return exec; }
std::string_view DesktopEntry::getIconId() const { return icon; }
bool DesktopEntry::needsTerminal() const { return useTerminal; }
bool DesktopEntry::isHidden() const { return hidden; }

std::pair<std::string_view, std::vector<std::string_view>> const DesktopEntry::getCommand(std::string& parsedExec) {
	parsedExec.clear();
	bool escape = false;
	for (char c : exec) {
		if (!escape) {
			if (c == '%') escape = true;
			else parsedExec += c;
		} else {
			switch (c) {
			case '%':
				parsedExec += '%';
				break;
			case 'f':
			case 'F':
			case 'u':
			case 'U':
				break;
			case 'i':
				parsedExec += "--icon '" + icon + '\'';
				break;
			case 'c':
				parsedExec += name;
				break;
			case 'k':
				parsedExec += path.native();
				break;
			default: break;
			}
			escape = false;
		}
	}

	std::vector<std::string_view> args = useTerminal ? TERMINAL_ARGS : SHELL_ARGS;
	std::string_view cmd = useTerminal ? TERMINAL : SHELL;

	return { cmd, args };
}

bool DesktopEntry::operator==(const DesktopEntry& other) const { return id == other.id; }
bool DesktopEntry::operator!=(const DesktopEntry& other) const { return !(operator==(other)); }

// ==========================================
// DesktopEntries
// ==========================================

std::string DesktopEntries::getEnviroment(std::string_view name) {
	char* val = getenv(name.data());
	return val ? val : ""s;
}
std::vector<fs::path> DesktopEntries::getEntryPaths() {
	std::string data_dirs = getEnviroment("XDG_DATA_DIRS"sv);
	std::vector<fs::path> out;
	if (!data_dirs.empty()) {
		auto pathbeg = ::begin(data_dirs);
		auto pathend = ::begin(data_dirs);
		while ((pathend = std::find(pathend, ::end(data_dirs), ':')) != ::end(data_dirs)) {
			out.emplace_back(pathbeg, pathend) /= "applications";
			pathbeg = ++pathend;
		}
		return out;
	}
	std::vector<fs::path> globalPaths = { "/usr/local/share/applications", "/usr/share/applications" };
	std::vector<fs::path> userPaths = { ".local/share/applications", ".data/applications" };
	fs::path home = getEnviroment("HOME"sv);
	for (auto& path : userPaths) path = home / path;
	out.insert(::end(out), ::begin(globalPaths), ::end(globalPaths));
	out.insert(::end(out), ::begin(userPaths), ::end(userPaths));
	return out;
}

std::vector<DesktopEntry> DesktopEntries::getDesktopEntries(const std::vector<fs::path> entryPaths) {
	std::vector<DesktopEntry> out;
	for (const auto& entryDirectory : entryPaths) {
		auto diriter = fs::recursive_directory_iterator(entryDirectory);
		for (const auto& file : diriter) {
			const auto path = file.path();
			if (!file.is_regular_file() || path.extension() != ".desktop") continue;
			DesktopEntry entry(entryDirectory, path);
			if (!entry.isHidden() && std::find(::begin(out), ::end(out), entry) == ::end(out))
				out.emplace_back(std::move(entry));
		}
	}
	std::sort(::begin(out), ::end(out), [](const auto& a, const auto& b){
			return a.getName() < b.getName();
	});
	return out;
}

DesktopEntries::DesktopEntries() : entries(getDesktopEntries(getEntryPaths())) {}
std::vector<DesktopEntry>::const_iterator DesktopEntries::begin() const { return ::begin(entries); }
std::vector<DesktopEntry>::const_iterator DesktopEntries::end() const { return ::end(entries); }
DesktopEntry DesktopEntries::operator[](int i) const { return entries[i]; }
