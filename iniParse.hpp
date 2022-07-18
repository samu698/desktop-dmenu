#pragma once

#include <string>
#include <string_view>
#include <vector>

// FIXME: this is not compliant with the freedesktop spec
// https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s03.html

class iniFile {
	struct iniEntry {
		std::string name, value; 

		iniEntry();
		iniEntry(std::string_view name);

		void clear();
		void trim();

		bool operator==(const iniEntry& other) const;
		bool operator!=(const iniEntry& other) const;
	};
	struct iniSection { 
		std::string section;
		std::vector<iniEntry> entries; 

		iniSection();
		iniSection(std::string_view section);

		void clear();

		bool operator==(const iniSection& other) const;
		bool operator!=(const iniSection& other) const;
	};

	std::vector<iniSection> sections;

	std::vector<iniSection> parseFile(std::string_view path);
public:
	iniFile(std::string_view path);
	std::vector<iniSection>::const_iterator begin() const;
	std::vector<iniSection>::const_iterator end() const;
};
