#include <iostream>
#include "desktopEntries.hpp"
#include "icons.hpp"

int main(int argc, const char* argv[]) {
	Icons i;
	std::cout << i.queryIcon("gparted", 48).getPath() << '\n';
	std::cout << i.queryIcon("mpv", 64).getPath() << '\n';
}
