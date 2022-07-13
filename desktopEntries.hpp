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
#include "execWrapper.hpp"
#include "config.h"

// https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

namespace fs = std::filesystem;
using namespace std::literals;

class DesktopEntry {
	std::string id;
	fs::path path;

	std::string name;
	std::string exec;
	std::string icon;
	bool useTerminal;
	bool hidden = false;

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
			else if (ename == "NoDisplay") hidden |= value == "true";
			else if (ename == "Hidden") hidden |= value == "true";
		}
	}

	std::string_view getId() const { return id; }
	const fs::path& getPath() const { return path; }
	std::string_view getName() const { return name; }
	std::string_view getExec() const { return exec; }
	std::string_view getIconId() const { return icon; }
	bool needsTerminal() const { return useTerminal; }
	bool isHidden() const { return hidden; }

	void run() {
		std::string parsedExec;
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
					parsedExec += "--icon";
					parsedExec += '\'' + icon + '\'';
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

		std::string_view svParsedExec = parsedExec;
		size_t beg = 0;
		for (size_t i = 0; i < svParsedExec.length(); i++) {
			if (svParsedExec[i] == ' ') {
				args.push_back(svParsedExec.substr(beg, i - beg));
				beg = i + 1;
			}
		}
		if (beg < svParsedExec.length() - 1) args.push_back(svParsedExec.substr(beg));

		Process p(cmd, args);
		p.exec();
	}

	bool operator==(const DesktopEntry& other) const { return id == other.id; }
	bool operator!=(const DesktopEntry& other) const { return !(operator==(other)); }
};

class DesktopEntries {
	std::vector<DesktopEntry> entries;

	std::string getEnviroment(std::string_view name) {
		char* val = getenv(name.data());
		return val ? val : ""s;
	}
	std::vector<fs::path> getEntryPaths() {
		std::string data_dirs = getEnviroment("XDG_DATA_DIRS"sv);
		std::vector<fs::path> out;
		if (!data_dirs.empty()) {
			auto pathbeg = std::begin(data_dirs);
			auto pathend = std::begin(data_dirs);
			while ((pathend = std::find(pathend, std::end(data_dirs), ':')) != std::end(data_dirs)) {
				out.emplace_back(pathbeg, pathend) /= "applications";
				pathbeg = ++pathend;
			}
			return out;
		}
		std::vector<fs::path> globalPaths = { "/usr/local/share/applications", "/usr/share/applications" };
		std::vector<fs::path> userPaths = { ".local/share/applications", ".data/applications" };
		fs::path home = getEnviroment("HOME"sv);
		for (auto& path : userPaths) path = home / path;
		out.insert(std::end(out), std::begin(globalPaths), std::end(globalPaths));
		out.insert(std::end(out), std::begin(userPaths), std::end(userPaths));
		return out;
	}

	std::vector<DesktopEntry> getDesktopEntries(const std::vector<fs::path> entryPaths) {
		std::vector<DesktopEntry> out;
		for (const auto& entryDirectory : entryPaths) {
			auto diriter = fs::recursive_directory_iterator(entryDirectory);
			for (const auto& file : diriter) {
				const auto path = file.path();
				if (!file.is_regular_file() || path.extension() != ".desktop") continue;
				DesktopEntry entry(entryDirectory, path);
				if (!entry.isHidden() && std::find(std::begin(out), std::end(out), entry) == std::end(out))
					out.emplace_back(std::move(entry));
			}
		}
		std::sort(std::begin(out), std::end(out), [](const auto& a, const auto& b){ 
				return a.getName() < b.getName();
		});
		return out;
	}

public:
	DesktopEntries() : entries(getDesktopEntries(getEntryPaths())) {}
	const auto begin() const { return std::begin(entries); }
	const auto end() const { return std::end(entries); }
	DesktopEntry operator[](int i) const { return entries[i]; }
};
