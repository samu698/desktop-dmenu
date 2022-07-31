#pragma once
#include <png.h>

#include "utils.hpp"

class PngReader {
	fs::path path;
	std::vector<uint8_t> pixels;
	FILE* fp;
	png_struct* png;
	png_info* info;
	bool infoRead;

	void getImgInfo();
	void readPixels();
public:
	PngReader(const fs::path& path);
	~PngReader();

	const std::vector<uint8_t>& getPixels();
	std::vector<uint8_t> getScaledPixels(uint32_t newW, uint32_t newH);
	uint32_t getWidth();
	uint32_t getHeight();
	uint8_t getDepth();
	uint8_t getColorType();
};
