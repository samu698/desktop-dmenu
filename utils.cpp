#include "utils.hpp"

std::string getEnviroment(std::string_view name) {
	char* val = getenv(name.data());
	return val ? val : ""s;
}
