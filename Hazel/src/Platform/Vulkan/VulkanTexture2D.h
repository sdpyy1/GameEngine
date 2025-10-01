#pragma once
#include"Hazel/Renderer/Texture.h"
namespace Hazel {
	//// Texture也是个壳子，实际上是创建的Image对象
	//class VulkanTexture2D : public Texture2D ,public std::enable_shared_from_this<VulkanTexture2D>
	//{
	//public:
	//	VulkanTexture2D(const TextureSpecification& specification, const std::filesystem::path& filepath);
	//	VulkanTexture2D(const TextureSpecification& specification, Buffer1 data = Buffer1());
	//	~VulkanTexture2D() override;

	//	virtual void CreateFromFile(const TextureSpecification& specification, const std::filesystem::path& filepath) override;
	//	virtual void ReplaceFromFile(const TextureSpecification& specification, const std::filesystem::path& filepath) override;
	//	virtual void CreateFromBuffer(const TextureSpecification& specification, Buffer1 data = Buffer1()) override;

	//	virtual void Resize(const glm::uvec2& size) override;
	//	virtual void Resize(uint32_t width, uint32_t height) override;

	//	void Invalidate();

	//	virtual ImageFormat GetFormat() const override { return m_Specification.Format; }
	//	virtual uint32_t GetWidth() const override { return m_Specification.Width; }
	//	virtual uint32_t GetHeight() const override { return m_Specification.Height; }
	//	virtual glm::uvec2 GetSize() const override { return { m_Specification.Width, m_Specification.Height }; }

	//	virtual void Bind(uint32_t slot = 0) const override;

	//	virtual Ref_old<Image2D> GetImage() const override { return m_Image; }
	//	virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return m_Image.As<VulkanImage2D>()->GetDescriptorInfo(); }
	//	const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

	//	void Lock() override;
	//	void Unlock() override;

	//	Buffer1 GetWriteableBuffer() override;
	//	bool Loaded() const override { return m_Image && m_Image->IsValid(); }
	//	const std::filesystem::path& GetPath() const override;
	//	uint32_t GetMipLevelCount() const override;
	//	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	//	void GenerateMips();

	//	virtual uint64_t GetHash() const { return (uint64_t)m_Image.As<VulkanImage2D>()->GetDescriptorInfoVulkan().imageView; }

	//	void CopyToHostBuffer(Buffer1& buffer);
	//private:
	//	void SetData(Buffer1 buffer);
	//private:
	//	TextureSpecification m_Specification;
	//	std::filesystem::path m_Path;

	//	Buffer1 m_ImageData;

	//	Ref_old<Image2D> m_Image;
	//};



}

