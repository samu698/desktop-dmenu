#pragma once
#include <png.h>

#include "utils.hpp"

class PngReader {
	std::vector<uint8_t> pixels;
	uint32_t w, h;
	uint8_t colorType, depth;

public:
	PngReader(const fs::path& path);

	const std::vector<uint8_t>& getPixels() const;
	std::vector<uint8_t> getScaledPixels(uint32_t newW, uint32_t newH) const;
	uint32_t getWidth() const;
	uint32_t getHeight() const;
	uint8_t getDepth() const;
	uint8_t getColorType() const;
};
