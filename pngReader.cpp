#include "pngReader.hpp"
#include <png.h>
PngReader::PngReader(const fs::path& path) : path(path), infoRead(false) {}

void PngReader::getImgInfo() {
	if (infoRead) return;

	fp = fopen(path.c_str(), "rb");
	uint8_t header[8];
	if (!fp) throw std::invalid_argument("cannot open file");
	fread(header, 8, 1, fp);
	if (png_sig_cmp(header, 0, 8)) throw std::invalid_argument("file is not a PNG");

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) throw std::runtime_error("png_create_read_struct failed");
	info = png_create_info_struct(png);
	if (!info) throw std::runtime_error("png_create_info_struct failed");

	png_init_io(png, fp);
	png_set_sig_bytes(png, 8);
	
	png_read_info(png, info);

	infoRead = true;
}

void PngReader::readPixels() {	
	getImgInfo();
	if (!pixels.empty()) return;

	const uint32_t w = png_get_image_width(png, info);
	const uint32_t h = png_get_image_height(png, info);
	const uint8_t colorType = png_get_color_type(png, info);
	const uint8_t depth = png_get_bit_depth(png, info);

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
}

PngReader::~PngReader() {
	png_destroy_read_struct(&png, &info, nullptr);
	fclose(fp);
}

const std::vector<uint8_t>& PngReader::getPixels() { readPixels(); return pixels; }
std::vector<uint8_t> PngReader::getScaledPixels(uint32_t newW, uint32_t newH) {
	readPixels();
	const uint32_t w = png_get_image_width(png, info);
	const uint32_t h = png_get_image_height(png, info);

	if (newW == w && newH == h) return pixels;	

	float xratio = (float)w / newW;
	float yratio = (float)h / newH;
	std::vector<uint8_t> scaled;
	scaled.reserve(newW * newH * 4);
	
	for (uint32_t y = 0; y < newH; y++)
		for (uint32_t x = 0; x < newW; x++) {
			uint32_t index = (y * yratio * w + x * xratio) * 4;
			scaled.push_back(pixels[index + 0]);
			scaled.push_back(pixels[index + 1]);
			scaled.push_back(pixels[index + 2]);
			scaled.push_back(pixels[index + 3]);
		}

	return scaled;
}

uint32_t PngReader::getWidth() { getImgInfo(); return png_get_image_width(png, info); }
uint32_t PngReader::getHeight() { getImgInfo(); return png_get_image_height(png, info); }
uint8_t PngReader::getDepth() { getImgInfo(); return png_get_bit_depth(png, info); }
uint8_t PngReader::getColorType() { getImgInfo(); return png_get_color_type(png, info); }
