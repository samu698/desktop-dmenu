#include <iostream>
#include <stdexcept>
#include "desktopEntries.hpp"
#include "icons.hpp"
#include "process.hpp"
#include "utils.hpp"

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
	dmenu.stream().sendEOF();
	std::string output;
	std::getline(dmenu.stream(), output);

	if (dmenu.join() != 0) exit(1);

	int index = std::stoi(output);
	return entries[index];
}

int main(int argc, const char* argv[]) {
	DesktopEntries entries;
	Icons icons;

	DesktopEntry e = askDesktopEntry(entries, icons);

	std::string parsedExec;
	auto [ cmd, args ] = e.getCommand(parsedExec);
	args.push_back(parsedExec);
	Process p(cmd, args);
	p.exec();
	throw std::runtime_error("Cannot exec program");
}
