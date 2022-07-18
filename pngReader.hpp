#pragma once
#include <png.h>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class PngReader {
	std::vector<uint8_t> pixels;
	uint32_t w, h;
	uint8_t colorType, depth;

public:
	PngReader(const fs::path& path) {
		FILE* fp = fopen(path.c_str(), "rb");
		uint8_t header[8];
		if (!fp) throw std::invalid_argument("cannot open file");
		fread(header, 8, 1, fp);
		if (png_sig_cmp(header, 0, 8)) throw std::invalid_argument("file is not a PNG");

		png_struct* png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png) throw std::runtime_error("png_create_read_struct failed");
		png_info* info = png_create_info_struct(png);
		if (!info) throw std::runtime_error("png_create_info_struct failed");

		png_init_io(png, fp);
		png_set_sig_bytes(png, 8);
		
		png_read_info(png, info);
		w = png_get_image_width(png, info);
		h = png_get_image_height(png, info);
		colorType = png_get_color_type(png, info);
		depth = png_get_bit_depth(png, info);

		if (depth == 16) png_set_strip_16(png);

		if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
		else if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);

		if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);

		png_read_update_info(png, info);

		size_t rowbytes = png_get_rowbytes(png, info);
		pixels = std::vector<uint8_t>(h * rowbytes, 0);
		uint8_t* rows[h];
		for (uint32_t y = 0; y < h; y++) rows[y] = pixels.data() + rowbytes * y;
		png_read_image(png, rows);

		png_destroy_read_struct(&png, &info, nullptr);
		fclose(fp);
	}

	const std::vector<uint8_t>& getPixels() { return pixels; }
	uint32_t getWidth() { return w; }
	uint32_t getHeight() { return h; }
	uint8_t getDepth() { return depth; }
	uint8_t getColorType() { return colorType; }
};
