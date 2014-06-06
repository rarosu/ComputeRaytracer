#include "TGA.h"
#include <fstream>

Tga::Tga(const char* FilePath)
{
	std::fstream hFile(FilePath, std::ios::in | std::ios::binary);
	if (!hFile.is_open()){throw std::invalid_argument("File Not Found.");}

	std::uint8_t Header[18] = {0};
	std::vector<std::uint8_t> ImageData;
	static std::uint8_t DeCompressed[12] = {0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	static std::uint8_t IsCompressed[12] = {0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	hFile.read(reinterpret_cast<char*>(&Header), sizeof(Header));

	if (!std::memcmp(DeCompressed, &Header, sizeof(DeCompressed)))
	{
		BitsPerPixel = Header[16];
		width  = Header[13] * 0xFF + Header[12];
		height = Header[15] * 0xFF + Header[14];
		size  = ((width * BitsPerPixel + 31) / 32) * 4 * height;

		if ((BitsPerPixel != 24) && (BitsPerPixel != 32))
		{
			hFile.close();
			throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit Image.");
		}

		ImageData.resize(size);
		ImageCompressed = false;
		hFile.read(reinterpret_cast<char*>(ImageData.data()), size);
	}
	else if (!std::memcmp(IsCompressed, &Header, sizeof(IsCompressed)))
	{
		BitsPerPixel = Header[16];
		width  = Header[13] * 0xFF + Header[12];
		height = Header[15] * 0xFF + Header[14];
		size  = ((width * BitsPerPixel + 31) / 32) * 4 * height;

		if ((BitsPerPixel != 24) && (BitsPerPixel != 32))
		{
			hFile.close();
			throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit Image.");
		}

		RGB Pixel = {0};
		int CurrentByte = 0;
		std::size_t CurrentPixel = 0;
		ImageCompressed = true;
		std::uint8_t ChunkHeader = {0};
		int BytesPerPixel = (BitsPerPixel / 8);
		ImageData.resize(width * height * sizeof(RGB));

		do
		{
			hFile.read(reinterpret_cast<char*>(&ChunkHeader), sizeof(ChunkHeader));

			if(ChunkHeader < 128)
			{
				++ChunkHeader;
				for(int I = 0; I < ChunkHeader; ++I, ++CurrentPixel)
				{
					hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

					ImageData[CurrentByte++] = Pixel.RGBA.B;
					ImageData[CurrentByte++] = Pixel.RGBA.G;
					ImageData[CurrentByte++] = Pixel.RGBA.R;
					if (BitsPerPixel > 24) ImageData[CurrentByte++] = Pixel.RGBA.A;
				}
			}
			else
			{
				ChunkHeader -= 127;
				hFile.read(reinterpret_cast<char*>(&Pixel), BytesPerPixel);

				for(int I = 0; I < ChunkHeader; ++I, ++CurrentPixel)
				{
					ImageData[CurrentByte++] = Pixel.RGBA.B;
					ImageData[CurrentByte++] = Pixel.RGBA.G;
					ImageData[CurrentByte++] = Pixel.RGBA.R;
					if (BitsPerPixel > 24) ImageData[CurrentByte++] = Pixel.RGBA.A;
				}
			}
		} while(CurrentPixel < (width * height));
	}
	else
	{
		hFile.close();
		throw std::invalid_argument("Invalid File Format. Required: 24 or 32 Bit TGA File.");
	}

	hFile.close();
	this->Pixels = ImageData.data();
}
