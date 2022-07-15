#include "iniParse.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

// FIXME: this is not compliant with the freedesktop spec
// https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s03.html

// ==========================================
// iniFile::iniEntry
// ==========================================

iniFile::iniEntry::iniEntry() = default;
iniFile::iniEntry::iniEntry(std::string_view name) : name(name) {}

void iniFile::iniEntry::clear() {
	name.clear();
	value.clear();
}
void iniFile::iniEntry::trim() {
	name.substr(0, name.find_last_not_of("\t\n\v\f\r ") + 1);
	if (!value.empty()) value.substr(value.find_first_not_of("\t\n\v\f\r "));
}

bool iniFile::iniEntry::operator==(const iniEntry& other) const { return name == other.name; }
bool iniFile::iniEntry::operator!=(const iniEntry& other) const { return !(operator==(other)); }

// ==========================================
// iniFile::iniSection
// ==========================================

iniFile::iniSection::iniSection() = default;
iniFile::iniSection::iniSection(std::string_view section) : section(section) {}

void iniFile::iniSection::clear() {
	section.clear();
	entries.clear();
}

bool iniFile::iniSection::operator==(const iniSection& other) const { return section == other.section; }
bool iniFile::iniSection::operator!=(const iniSection& other) const { return !(operator==(other)); }

// ==========================================
// iniFile
// ==========================================

std::vector<iniFile::iniSection> iniFile::parseFile(std::string_view path) {
	std::vector<iniSection> tmp;

	std::ifstream file(path.data());

	enum { SECTION, OPT, OPT_VALUE, LINE_END } state;
	std::string line;
	iniSection section;
	iniEntry entry;

	for (;;) {
		if (!getline(file, line))  break;

		if (line.empty()) continue;

		switch (line[0]) {
			case '[':
				state = SECTION;
				sections.push_back(section);
				section.clear();
				break;
			case '#': 
				continue;
			default:
				state = OPT;
				entry.name += line[0];
				break;
		}

		for (size_t i = 1; i < line.size(); i++) {
			const char c = line[i];
			switch (state) {
			case SECTION:
				if (c == ']') state = LINE_END;
				else section.section += c;
				break;
			case OPT:
				if (c == '=') state = OPT_VALUE;
				else entry.name += c;
				break;
			case OPT_VALUE:
				entry.value += c;
				break;
			case LINE_END:
				// error?
				break;
			}
		}

		if (state == OPT_VALUE) {
			entry.trim();
			section.entries.push_back(entry);
			entry.clear();
		}
		// error
	}

	if (section.entries.size() != 0)
		sections.push_back(section);

	if (sections[0].entries.size() == 0)
		sections.erase(sections.begin());

	return tmp;
}
iniFile::iniFile(std::string_view path) : sections(parseFile(path)) {}
std::vector<iniFile::iniSection>::const_iterator iniFile::begin() const { return sections.begin(); }
std::vector<iniFile::iniSection>::const_iterator iniFile::end() const { return sections.end(); }
