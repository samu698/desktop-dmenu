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
#include <unordered_map>
#include <utility>

#include "iniParse.hpp"
#include "pngReader.hpp"

// https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
// https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

namespace fs = std::filesystem;
using namespace std::literals;

template<typename Iter>
struct iteratorHelper {
	Iter ibegin, iend;
	iteratorHelper(Iter begin, Iter end) : ibegin(begin), iend(end) {}
	iteratorHelper(std::pair<Iter, Iter> iters) : ibegin(iters.first), iend(iters.second) {}
	Iter begin() { return ibegin; }
	Iter end() { return iend; }
};

class Icon {
	std::string name;
	int size;
	fs::path path;

	std::string escapeData(const std::vector<uint8_t>& png) const {
		std::string out;
		out.reserve(png.size());
		for (auto b : png)
			switch (b) {
			case '\n':
				out += "\\n";
				break;
			case '\\':
				out += "\\\\";
				break;
			case '\0':
				out += "\\0";
				break;
			default:
				out += b;
			}
		return out;
	}
public:
	Icon(std::string_view name, int size) : name(name), size(size) {}
	Icon(std::string_view name, int size, fs::path path) : name(name), size(size), path(path) {}

	std::string_view getName() const { return name; }
	int getSize() const { return size; }
	const fs::path& getPath() const { return path; }
	void setPath(const fs::path& newPath) { path = newPath; }

	bool exists() const { return !path.empty(); }
	std::string dmenuString() const {
		PngReader png(path);
		return escapeData(png.getPixels());
	}
};


class IconTheme {
	std::string id;
	mutable std::unordered_multimap<int, fs::path> iconFolders;

	void readIndex(const std::vector<fs::path>& iconPaths) const {
		for (const auto& iconPath : iconPaths) {
			fs::path indexPath = iconPath / id / "index.theme";
			if (!fs::exists(indexPath)) continue;
			iniFile indexFile(indexPath.native());
			for (const auto& [ section, entries ] : indexFile) {
				bool validFolder = true;
				int size = 0;
				for (const auto& [ name, value ] : entries) {
					if (name == "Size") {
						size = stoi(value);
					} else if (name == "Type") {
						if (value == "Scalable") {
							validFolder = false;
							break;
						}
					} else if (name == "Scale") {
						if (stoi(value) != 1) {
							validFolder = false;
							break;
						}
					} else if (name == "Context") {
						if (value != "Applications") {
							validFolder = false;
							break;
						}
					}
				}
				if (!validFolder || size == 0) continue;
				iconFolders.emplace(size, section);
			}
		}
	}
public:
	IconTheme(std::string id) : id(id) {}
	void addIndex(const fs::path& index) {}

	std::string_view getId() const { return id; }

	fs::path queryIcon(std::string_view name, int size, const std::vector<fs::path>& iconPaths) const {
		if (iconFolders.empty()) readIndex(iconPaths);
		for (const auto& iconPath : iconPaths) {
			iteratorHelper folderRange = iconFolders.equal_range(size);
			for (const auto& [ _, relativeIconFolder ] : folderRange) {
				fs::path iconFolder = iconPath / id / relativeIconFolder;
				if (!fs::exists(iconFolder)) continue;
				auto diriter = fs::directory_iterator(iconFolder);
				for (const auto& iconFile : diriter) {
					if (!iconFile.is_regular_file()) continue;

					const auto& iconPath = iconFile.path();
					if (iconPath.extension() != ".png") continue;
					if (iconPath.stem() == name) return iconPath;
				}
			}
		}
		return "";
	}

	bool operator==(const IconTheme& other) const { return id == other.id; }
	bool operator!=(const IconTheme& other) const { return !(operator==(other)); }
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

	std::string getEnviroment(std::string_view name) {
		char* val = getenv(name.data());
		return val ? val : ""s;
	}

	std::vector<fs::path> getIconPaths() {
		std::vector<fs::path> out = { getEnviroment("HOME"sv) + "/.icons" };
		std::string data_dirs = getEnviroment("XDG_DATA_DIRS"sv);
		if (!data_dirs.empty()) {
			auto pathbeg = begin(data_dirs);
			auto pathend = begin(data_dirs);
			while ((pathend = std::find(pathend, end(data_dirs), ':')) != end(data_dirs)) {
				out.emplace_back(pathbeg, pathend) /= "icons";
				pathbeg = ++pathend;
			}
		} else {
			fs::path home = getEnviroment("HOME"sv);
			out.emplace_back("/usr/local/share/icons/"sv);
			out.emplace_back("/usr/share/icons/"sv);
			out.emplace_back(home / ".local/share/icons");
			out.emplace_back(home / ".data/icons");
		}
		return out;
	}

	std::unordered_set<IconTheme> getThemes(const std::vector<fs::path>& iconPaths) {
		std::unordered_set<IconTheme> themes;
		for (const auto& iconPath : iconPaths) {
			if (!fs::exists(iconPath)) continue;
			auto diriter = fs::directory_iterator(iconPath);
			for (const auto& themeFolder : diriter) {
				if (!themeFolder.is_directory()) continue;
				std::string themeName = themeFolder.path().filename();
				if (themes.count(themeName) == 0) {
					themes.emplace(themeName);
				}
			}
		}
		return themes;
	}
public:
	Icons() : iconPaths(getIconPaths()), themes(getThemes(iconPaths)) {}

	std::optional<Icon> queryIcon(std::string_view name, int size, const std::string& preferredThemeId = "") {
		Icon icon(name, size);

		/* query current theme */
		if (!preferredThemeId.empty()) {
			auto preferredTheme = themes.find(IconTheme(preferredThemeId));
			if (preferredTheme != themes.end())
				icon.setPath(preferredTheme->queryIcon(name, size, iconPaths));
			if (icon.exists()) return icon;
		}

		auto hicolorTheme = themes.find(IconTheme("hicolor"));
		if (hicolorTheme != themes.end())
			icon.setPath(hicolorTheme->queryIcon(name, size, iconPaths));
		if (icon.exists()) return icon;

		/* TODO: query /usr/share/pixmaps/ folder */;

		return std::nullopt;
	}
};
