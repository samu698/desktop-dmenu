#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "config.h"

using namespace std::literals;
namespace fs = std::filesystem;
using std::begin;
using std::end;

std::string getEnviroment(std::string_view name);

template<typename T> std::vector<T> prepend(std::vector<T> vec, T val) {
	vec.insert(std::begin(vec), val);
	return vec;
}

template<typename Iter>
struct pairIterator {
	Iter ibegin, iend;
	pairIterator(Iter begin, Iter end) : ibegin(begin), iend(end) {}
	pairIterator(std::pair<Iter, Iter> iters) : ibegin(iters.first), iend(iters.second) {}
	Iter begin() { return ibegin; }
	Iter end() { return iend; }
};
