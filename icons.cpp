#include "icons.hpp"
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <optional>

#include "iniParse.hpp"
#include "pngReader.hpp"
#include "utils.hpp"

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
Icon::Icon(std::string_view name) : name(name) {}
Icon::Icon(std::string_view name, uint32_t size, fs::path path) : name(name), size(size), path(path) {}
std::string_view Icon::getName() const { return name; }
uint32_t Icon::getSize() const { return size; }
const fs::path& Icon::getPath() const { return path; }
std::string Icon::dmenuString() const {
	PngReader png(path);
	return escapeData(png.getScaledPixels(16, 16));
}
bool Icon::operator==(const Icon& other) const { return name == other.name; }
bool Icon::operator!=(const Icon& other) const { return !operator==(other); }

// ==========================================
// IconTheme
// ==========================================

void IconTheme::indexIcons(const std::vector<fs::path>& iconPaths) const {
	std::vector<std::pair<int, fs::path>> relativePaths;
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
			relativePaths.emplace_back(size, section);
		}
	}
	for (const auto& iconPath : iconPaths) {
		for (const auto& [size, relativePath] : relativePaths) {
			fs::path p = iconPath / id / relativePath;
			if (!fs::exists(p)) continue;
			for (const auto& iconFile : fs::directory_iterator(p)) {
				if (!iconFile.is_regular_file()) continue;

				const auto& iconPath = iconFile.path();
				if (iconPath.extension() != ".png") continue;
				icons.emplace(iconPath.stem().native(), size, iconPath);
			}
		}
	}
}
IconTheme::IconTheme(std::string id) : id(id) {}
std::string_view IconTheme::getId() const { return id; }
std::vector<Icon> IconTheme::queryIcons(std::string_view name, const std::vector<fs::path>& iconPaths) const {
	if (icons.empty()) indexIcons(iconPaths);
	std::vector<Icon> found;
	const auto& [ beg, end ] = icons.equal_range(Icon(name));
	if (beg != ::end(icons))
		std::copy(beg, end, std::back_inserter(found));
	return found;
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
std::vector<Icon> Icons::queryIcons(std::string_view name, std::string_view preferredThemeId) {
	std::vector<Icon> icons;
	if (!preferredThemeId.empty()) {
		auto preferredTheme = themes.find(IconTheme(std::string(preferredThemeId)));
		if (preferredTheme != themes.end())
			icons = preferredTheme->queryIcons(name, iconPaths);
		if (!icons.empty()) return icons;
	}

	auto hicolorTheme = themes.find(IconTheme("hicolor"));
	if (hicolorTheme != themes.end())
		icons = hicolorTheme->queryIcons(name, iconPaths);

	if (!icons.empty()) return icons;

	for (auto icon : fs::directory_iterator("/usr/share/pixmaps")) {
		if (icon.path().stem() != name) continue;
		if (icon.path().extension() != ".png") continue;
		PngReader p(icon.path());
		if (p.getWidth() != p.getHeight()) continue;
		icons.emplace_back(name, p.getWidth(), icon.path());
	}

	return icons;
}

// This function scores the icon sizes, based on which is better to be resized to the desired size.
// The factors that this function considers are: (in order of imporance)
// 1. Is the icon bigger/smaller than the desired size (downscaling is better)
// 2. Is the icon a multiple of the desired size
// 3. How big is the difference between the two sizes
int scoreSize(uint32_t desired, uint32_t size) {
	if (size == desired) return 1000000;
	else if (size % desired == 0) return 1000000 - size / desired;
	else if (desired % size == 0) return -(desired / size);
	else if (size > desired) return size - desired;
	else return -1000000 + desired - size;
}

std::optional<Icon> Icons::queryIconClosestSize(std::string_view name, uint32_t size, std::string_view preferredThemeId) {
	auto icons = queryIcons(name, preferredThemeId);
	if (icons.empty()) return std::nullopt;
	std::vector<int> scores(icons.size());
	std::transform(begin(icons), end(icons), begin(scores), [size](const auto& e) { return scoreSize(size, e.getSize()); });
	auto it = std::max_element(begin(scores), end(scores));

	return icons[std::distance(begin(scores), it)];
}
