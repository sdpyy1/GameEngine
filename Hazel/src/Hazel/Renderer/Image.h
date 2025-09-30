#pragma once
#include "RendererResource.h"
#include <Hazel/Core/Buffer1.h>

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

		B10R11G11UF,

		SRGB,
		SRGBA,

		DEPTH32FSTENCIL8UINT,
		DEPTH32F,
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8,
	};

	// ͼƬ��;
	enum class ImageUsage
	{
		None = 0,
		Texture,
		Attachment,
		Storage,
		HostRead
	};

	// �����Ʒ�ʽ
	enum class TextureWrap
	{
		None = 0,
		Clamp,
		Repeat
	};

	// ������˷�ʽ
	enum class TextureFilter
	{
		None = 0,
		Linear,
		Nearest,
		Cubic
	};

	// ��������
	enum class TextureType
	{
		None = 0,
		Texture2D,
		TextureCube
	};

	// ͼƬ���
	struct ImageSpecification
	{
		std::string DebugName;

		ImageFormat Format = ImageFormat::RGBA;
		ImageUsage Usage = ImageUsage::Texture;
		bool Transfer = false; // Will it be used for transfer ops?
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Mips = 1;
		uint32_t Layers = 1;
		bool CreateSampler = true;
	};

	struct ImageSubresourceRange
	{
		uint32_t BaseMip = 0; // ��ʼmip��
		uint32_t MipCount = UINT_MAX; // ����mip��
		uint32_t BaseLayer = 0; // ��ʼ��ͼ�㣨Layer��������ͼ��ͨ����������������6 ���棩�������������ͼ��ļ��ϣ��ȳ�����
		uint32_t LayerCount = UINT_MAX; // ��Ҫ������ͼ��������
	};

	// ���ֵ
	union ImageClearValue
	{
		glm::vec4 FloatValues;
		glm::ivec4 IntValues;
		glm::uvec4 UIntValues;
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

		virtual Buffer1 GetBuffer() const = 0;
		virtual Buffer1& GetBuffer() = 0;

		virtual uint64_t GetGPUMemoryUsage() const = 0;

		virtual void CreatePerLayerImageViews() = 0;

		virtual uint64_t GetHash() const = 0;

		virtual void SetData(Buffer1 buffer) = 0;
		virtual void CopyToHostBuffer(Buffer1& buffer) const = 0;

		// TODO: usage (eg. shader read)
	};

	class Image2D : public Image
	{
	public:
		static Ref<Image2D> Create(const ImageSpecification& specification, Buffer1 buffer = Buffer1());
		virtual void Resize(const glm::uvec2& size) = 0;
		virtual bool IsValid() const = 0;
	};

}
