#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

// https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
// https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

namespace fs = std::filesystem;

class Icon {
	std::string name;
	int size;
	fs::path path;

	std::string escapeData(const std::vector<uint8_t>& png) const;
public:
	Icon(std::string_view name, int size);
	Icon(std::string_view name, int size, fs::path path);

	std::string_view getName() const;
	int getSize() const;
	const fs::path& getPath() const;
	void setPath(const fs::path& newPath);

	bool exists() const;
	std::string dmenuString() const;
};

class IconTheme {
	std::string id;
	mutable std::unordered_multimap<int, fs::path> iconFolders;

	void readIndex(const std::vector<fs::path>& iconPaths) const;
public:
	IconTheme(std::string id);
	void addIndex(const fs::path& index);

	std::string_view getId() const;

	fs::path queryIcon(std::string_view name, int size, const std::vector<fs::path>& iconPaths) const;

	bool operator==(const IconTheme& other) const;
	bool operator!=(const IconTheme& other) const;
};

namespace std {
template<> struct hash<IconTheme> {
	size_t operator()(const IconTheme& i) const {
		return hash<string_view>{}(i.getId());
	}
};
}

class Icons {
	std::vector<fs::path> iconPaths;
	std::unordered_set<IconTheme> themes;

	std::string getEnviroment(std::string_view name);

	std::vector<fs::path> getIconPaths();
	std::unordered_set<IconTheme> getThemes(const std::vector<fs::path>& iconPaths);
public:
	Icons();

	std::optional<Icon> queryIcon(std::string_view name, int size, const std::string& preferredThemeId = "");
};
