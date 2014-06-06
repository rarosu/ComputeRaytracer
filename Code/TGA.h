#pragma once

#include <vector>
#include <cstdint>

typedef union RGBA
{
	std::uint32_t Colour;
	struct
	{
		std::uint8_t B, G, R, A;
	};
} *PRGB;

class Tga
{
private:
	std::vector<unsigned char> Pixels;
	bool ImageCompressed;
	std::uint32_t width, height, size, BitsPerPixel;

public:
	Tga(const char* FilePath);
	std::vector<unsigned char> GetPixels() {return this->Pixels;}
};



