#pragma once

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

#include "iniParse.hpp"

// https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

namespace fs = std::filesystem;
using namespace std::literals;

class DesktopEntry {
	std::string id;
	fs::path path;

	std::string name;
	std::string exec;
	bool useTerminal;
	std::string icon;

	std::string pathToId(const fs::path& base, const fs::path& path) {
		fs::path relative = path.lexically_relative(base);
		std::string id;
		auto length = std::distance(relative.begin(), relative.end());
		auto last = std::for_each_n(relative.begin(), length - 1, [&id](const auto& p) {
			id += p.native() + '-';
		});
		id += *last;
		return id;
	}
public:
	DesktopEntry(std::string_view id) : id(id) {}
	DesktopEntry(const fs::path& base, const fs::path& path) : path(path), id(pathToId(base, path)) {
		iniFile desktopFile(path.native());
		auto section = std::find(desktopFile.begin(), desktopFile.end(), "Desktop Entry"sv);
		for (const auto& [ ename, value ] : section->entries) {
			if (ename == "Name") name = value;
			else if (ename == "Icon") icon = value;
			else if (ename == "Exec") exec = value;
			else if (ename == "Terminal") useTerminal = value == "true";
		}
	}

	std::string_view getId() const { return id; }
	const fs::path& getPath() const { return path; }
	std::string_view getName() const { return name; }
	std::string_view getExec() const { return exec; }
	std::string_view getIconId() const { return icon; }
	bool needsTerminal() const { return useTerminal; }

	bool operator==(const DesktopEntry& other) const { return id == other.id; }
	bool operator!=(const DesktopEntry& other) const { return !(operator==(other)); }
};

namespace std {
template<> struct hash<DesktopEntry> {
	size_t operator()(const DesktopEntry& d) const {
		return hash<string_view>{}(d.getId());
	}
};
}

class DesktopEntries {
	std::unordered_set<DesktopEntry> entries;

	std::string getEnviroment(std::string_view name) {
		char* val = getenv(name.data());
		return val ? val : ""s;
	}
	std::vector<fs::path> getEntryPaths() {
		std::string data_dirs = getEnviroment("XDG_DATA_DIRS"sv);
		if (!data_dirs.empty()) {
			std::vector<fs::path> out;
			auto pathbeg = std::begin(data_dirs);
			auto pathend = std::begin(data_dirs);
			while ((pathend = std::find(pathend, std::end(data_dirs), ':')) != std::end(data_dirs)) {
				out.emplace_back(pathbeg, pathend) /= "applications";
				pathbeg = ++pathend;
			}
			return out;
		}
		return { "/usr/local/share/applications", "/usr/share/applications" };
	}

	std::unordered_set<DesktopEntry> getDesktopEntries(const std::vector<fs::path> entryPaths) {
		std::unordered_set<DesktopEntry> out;
		for (const auto& entryDirectory : entryPaths) {
			auto diriter = fs::recursive_directory_iterator(entryDirectory);
			for (const auto& file : diriter) {
				const auto path = file.path();
				if (!file.is_regular_file() || path.extension() != ".desktop") continue;
				DesktopEntry entry(entryDirectory, path);
				if (out.count(entry) == 0) 
					out.emplace(std::move(entry));
			}
		}
		return out;
	}
public:
	DesktopEntries() : entries(getDesktopEntries(getEntryPaths())) {}
	const auto begin() { return std::begin(entries); }
	const auto end()  { return  std::end(entries); }
};
