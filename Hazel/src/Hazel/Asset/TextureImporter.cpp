#include "hzpch.h"
#include "TextureImporter.h"
#include <Hazel/Utils/FileSystem.h>
#include <stb_image.h>
namespace Hazel {

	Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path& path, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		FileStatus fileStatus = FileSystem::TryOpenFileAndWait(path, 100);
		Buffer imageBuffer;
		std::string pathString = path.string();
		bool isSRGB = (outFormat == ImageFormat::SRGB) || (outFormat == ImageFormat::SRGBA);
		int width, height, channels;
		if (stbi_is_hdr(pathString.c_str()))
		{
			imageBuffer.Data = (byte*)stbi_loadf(pathString.c_str(), &width, &height, &channels, 4);
			imageBuffer.Size = width * height * 4 * sizeof(float);
			outFormat = ImageFormat::RGBA32F;
		}
		else
		{
			stbi_set_flip_vertically_on_load(1);
			imageBuffer.Data = stbi_load(pathString.c_str(), &width, &height, &channels, 4);
			imageBuffer.Size = width * height * 4;
			outFormat = isSRGB ? ImageFormat::SRGBA : ImageFormat::RGBA;
		}

		if (!imageBuffer.Data)
			return {};

		outWidth = width;
		outHeight = height;
		return imageBuffer;
	}

	Buffer TextureImporter::ToBufferFromMemory(Buffer buffer, ImageFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		Buffer imageBuffer;

		int width, height, channels;
		bool isSRGB = (outFormat == ImageFormat::SRGB) || (outFormat == ImageFormat::SRGBA);
		if (stbi_is_hdr_from_memory((const stbi_uc*)buffer.Data, (int)buffer.Size))
		{
			imageBuffer.Data = (byte*)stbi_loadf_from_memory((const stbi_uc*)buffer.Data, (int)buffer.Size, &width, &height, &channels, STBI_rgb_alpha);
			imageBuffer.Size = width * height * 4 * sizeof(float);
			outFormat = ImageFormat::RGBA32F;
		}
		else
		{
			// stbi_set_flip_vertically_on_load(1);
			imageBuffer.Data = stbi_load_from_memory((const stbi_uc*)buffer.Data, (int)buffer.Size, &width, &height, &channels, STBI_rgb_alpha);
			imageBuffer.Size = width * height * 4;
			outFormat = isSRGB ? ImageFormat::SRGBA : ImageFormat::RGBA;
		}

		if (!imageBuffer.Data)
			return {};

		outWidth = width;
		outHeight = height;
		return imageBuffer;
	}

}

