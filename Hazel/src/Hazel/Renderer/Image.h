#pragma once
#include "RendererResource.h"
#include <Hazel/Core/Buffer.h>
#include <glm/gtc/integer.hpp>
namespace Hazel {
	enum class ImageFormat
	{
		None = 0,
		RED8UN,
		RED8UI,
		RED16UI,
		RED32UI,
		RED32F,
		RG8,
		RG16F,
		RG32F,
		RGB,
		RGBA,
		RGBA16F,
		RGBA32F,
		RGB16F,
		RGB32F,
		B10R11G11UF,
		RGB8,
		SRGB,
		SRGBA,
		SRGB8,
		DEPTH32FSTENCIL8UINT,
		DEPTH32F,
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH32F,
	};

	// 图片用途
	enum class ImageUsage
	{
		None = 0,
		Texture,
		Attachment,
		Storage,
		HostRead
	};

	// 纹理环绕方式
	enum class TextureWrap
	{
		None = 0,
		Clamp,
		Repeat
	};

	// 纹理过滤方式
	enum class TextureFilter
	{
		None = 0,
		Linear,
		Nearest,
		Cubic
	};

	// 纹理类型
	enum class TextureType
	{
		None = 0,
		Texture2D,
		TextureCube
	};

	// 图片规格
	struct ImageSpecification
	{
		std::string DebugName;

		ImageFormat Format = ImageFormat::RGBA;
		ImageUsage Usage = ImageUsage::Texture;
		bool Transfer = false;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Mips = 1;
		uint32_t Layers = 1;
		bool CreateSampler = true;
	};

	class Image : public RendererResource
	{
	public:
		virtual ~Image() = default;

		virtual void Resize(const uint32_t width, const uint32_t height) = 0;
		virtual void Invalidate() = 0;
		virtual void Release() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual glm::uvec2 GetSize() const = 0;
		virtual bool HasMips() const = 0;

		virtual float GetAspectRatio() const = 0;

		virtual ImageSpecification& GetSpecification() = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

		virtual Buffer GetBuffer() const = 0;
		virtual Buffer& GetBuffer() = 0;

		virtual uint64_t GetGPUMemoryUsage() const = 0; 

		virtual void CreatePerLayerImageViews() = 0;

		virtual uint64_t GetHash() const = 0;

		virtual void SetData(Buffer buffer) = 0;
		virtual void CopyToHostBuffer(Buffer& buffer) const = 0; 
	};

	class Image2D : public Image
	{
	public:
		static Ref<Image2D> Create(const ImageSpecification& specification, Buffer buffer = Buffer());
		virtual void Resize(const glm::uvec2& size) = 0;
		virtual bool IsValid() const = 0;
	};
	namespace Utils {
		inline uint32_t GetImageFormatBPP(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RED8UN:  return 1;
			case ImageFormat::RED8UI:  return 1;
			case ImageFormat::RED16UI: return 2;
			case ImageFormat::RED32UI: return 4;
			case ImageFormat::RED32F:  return 4;
			case ImageFormat::RGB:
			case ImageFormat::SRGB:    return 3;
			case ImageFormat::RGBA:    return 4;
			case ImageFormat::SRGBA:   return 4;
			case ImageFormat::RGBA16F: return 2 * 4;
			case ImageFormat::RGBA32F: return 4 * 4;
			case ImageFormat::B10R11G11UF: return 4;
			}
			HZ_CORE_ASSERT(false);
			return 0;
		}

		inline bool IsIntegerBased(const ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RED16UI:
			case ImageFormat::RED32UI:
			case ImageFormat::RED8UI:
			case ImageFormat::DEPTH32FSTENCIL8UINT:
				return true;
			case ImageFormat::DEPTH32F:
			case ImageFormat::RED8UN:
			case ImageFormat::RGBA32F:
			case ImageFormat::B10R11G11UF:
			case ImageFormat::RG16F:
			case ImageFormat::RG32F:
			case ImageFormat::RED32F:
			case ImageFormat::RG8:
			case ImageFormat::RGBA:
			case ImageFormat::RGBA16F:
			case ImageFormat::RGB:
			case ImageFormat::SRGB:
			case ImageFormat::SRGBA:
			case ImageFormat::DEPTH24STENCIL8:
				return false;
			}
			HZ_CORE_ASSERT(false);
			return false;
		}

		inline uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			if (width == 0 || height == 0) return 1;
			uint32_t mipCount = 1;
			uint32_t currentMin = glm::min(width, height);
			while (currentMin > 1) {
				currentMin = (currentMin + 1) / 2; 
				mipCount++;
			}
			return mipCount;
		}

		inline uint32_t GetImageMemorySize(ImageFormat format, uint32_t width, uint32_t height)
		{
			return width * height * GetImageFormatBPP(format);
		}

		inline bool IsDepthFormat(ImageFormat format)
		{
			if (format == ImageFormat::DEPTH24STENCIL8 || format == ImageFormat::DEPTH32F || format == ImageFormat::DEPTH32FSTENCIL8UINT  || format == ImageFormat::Depth)
				return true;

			return false;
		}
	}

	struct ImageViewSpecification
	{
		Ref<Image2D> Image;
		uint32_t Mip = 0;

		std::string DebugName;
	};

	class ImageView : public RendererResource
	{
	public:
		virtual ~ImageView() = default;

		static Ref<ImageView> Create(const ImageViewSpecification& specification);
	};
}
