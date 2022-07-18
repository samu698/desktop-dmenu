#include "icons.hpp"
#include <algorithm>
#include <optional>

#include "iniParse.hpp"
#include "pngReader.hpp"

// https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
// https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html

// ==========================================
// Icon
// ==========================================

std::string Icon::escapeData(const std::vector<uint8_t>& pixels) const {
	std::string out;
	out.reserve(pixels.size());
	for (auto b : pixels)
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
Icon::Icon(std::string_view name, int size) : name(name), size(size) {}
Icon::Icon(std::string_view name, int size, fs::path path) : name(name), size(size), path(path) {}
std::string_view Icon::getName() const { return name; }
int Icon::getSize() const { return size; }
const fs::path& Icon::getPath() const { return path; }
void Icon::setPath(const fs::path& newPath) { path = newPath; }
bool Icon::exists() const { return !path.empty(); }
std::string Icon::dmenuString() const {
	PngReader png(path);
	return escapeData(png.getPixels());
}

// ==========================================
// IconTheme
// ==========================================

void IconTheme::readIndex(const std::vector<fs::path>& iconPaths) const {
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
IconTheme::IconTheme(std::string id) : id(id) {}
void IconTheme::addIndex(const fs::path& index) {}
std::string_view IconTheme::getId() const { return id; }
fs::path IconTheme::queryIcon(std::string_view name, int size, const std::vector<fs::path>& iconPaths) const {
	if (iconFolders.empty()) readIndex(iconPaths);
	for (const auto& iconPath : iconPaths) {
		pairIterator folderRange = iconFolders.equal_range(size);
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
bool IconTheme::operator==(const IconTheme& other) const { return id == other.id; }
bool IconTheme::operator!=(const IconTheme& other) const { return !(operator==(other)); }

// ==========================================
// Icons
// ==========================================

std::vector<fs::path> Icons::getIconPaths() {
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
std::unordered_set<IconTheme> Icons::getThemes(const std::vector<fs::path>& iconPaths) {
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
Icons::Icons() : iconPaths(getIconPaths()), themes(getThemes(iconPaths)) {}
std::optional<Icon> Icons::queryIcon(std::string_view name, int size, const std::string& preferredThemeId) {
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
