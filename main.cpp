#include <iostream>
#include <stdexcept>
#include "desktopEntries.hpp"
#include "icons.hpp"
#include "execWrapper.hpp"
#include "config.h"

DesktopEntry askDesktopEntry(DesktopEntries& entries, Icons& icons) {
	Process dmenu("dmenu", DMENU_ARGS);
	dmenu.run();
	for (const auto& entry : entries) {
		auto icon = icons.queryIcon(entry.getIconId(), 16);
		dmenu.stream() << entry.getName();
		if (icon) {
			dmenu.stream().write("\0", 1);
			dmenu.stream() << icon->dmenuString();
		}
		dmenu.stream() << '\n';
	}
	dmenu.stream().sendEof();
	std::string output;
	std::getline(dmenu.stream(), output);
	dmenu.join();

	int index;
	try {
		index = std::stoi(output);
	} catch (std::invalid_argument e) {
		exit(1);
	}
	return entries[index];
}

int main(int argc, const char* argv[]) {
	DesktopEntries entries;
	Icons icons;

	DesktopEntry e = askDesktopEntry(entries, icons);

	e.run();
}
