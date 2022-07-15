#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <iterator>

// https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

class DesktopEntry {
	std::string id;
	std::filesystem::path path;

	std::string name;
	std::string exec;
	std::string icon;
	bool useTerminal;
	bool hidden = false;

	std::string pathToId(const std::filesystem::path& base, const std::filesystem::path& path);
public:
	DesktopEntry(std::string_view id);
	DesktopEntry(const std::filesystem::path& base, const std::filesystem::path& path);

	std::string_view getId() const;
	const std::filesystem::path& getPath() const;
	std::string_view getName() const;
	std::string_view getExec() const;
	std::string_view getIconId() const;
	bool needsTerminal() const;
	bool isHidden() const;

	void run();

	bool operator==(const DesktopEntry& other) const;
	bool operator!=(const DesktopEntry& other) const;
};

class DesktopEntries {
	std::vector<DesktopEntry> entries;

	std::string getEnviroment(std::string_view name);
	std::vector<std::filesystem::path> getEntryPaths();

	std::vector<DesktopEntry> getDesktopEntries(const std::vector<std::filesystem::path> entryPaths);

public:
	DesktopEntries();
	std::vector<DesktopEntry>::const_iterator begin() const;
	std::vector<DesktopEntry>::const_iterator end() const;
	DesktopEntry operator[](int i) const;
};
