#pragma once

#include <optional>
#include "utils.hpp"

// https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
// https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

class Icon {
	std::string name;
	uint32_t size;
	fs::path path;

	std::string escapeData(const std::vector<uint8_t>& png) const;
public:
	Icon(std::string_view name);
	Icon(std::string_view name, uint32_t size, fs::path path);

	std::string_view getName() const;
	uint32_t getSize() const;
	const fs::path& getPath() const;
	std::string dmenuString() const;

	bool operator==(const Icon& other) const;
	bool operator!=(const Icon& other) const;
};

namespace std {
template<> struct hash<Icon> {
	size_t operator()(const Icon& i) const {
		return hash<string_view>{}(i.getName());
	}
};
}

class IconTheme {
	std::string id;
	mutable std::unordered_multiset<Icon> icons;

	void findIcons(const std::vector<fs::path>& iconPaths) const;
public:
	IconTheme(std::string id);
	void addIndex(const fs::path& index);

	std::string_view getId() const;

	std::vector<Icon> queryIcons(std::string_view name, const std::vector<fs::path>& iconPaths) const;

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

	std::vector<fs::path> getIconPaths();
	std::unordered_set<IconTheme> getThemes(const std::vector<fs::path>& iconPaths);
public:
	Icons();

	std::vector<Icon> queryIcons(std::string_view name, std::string_view preferredThemeId = "");
	std::optional<Icon> queryIconClosestSize(std::string_view name, uint32_t size, std::string_view preferredThemeId = "");
};
