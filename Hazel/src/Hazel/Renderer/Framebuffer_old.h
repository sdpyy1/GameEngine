#pragma once

#include "Hazel/Core/Base.h"

namespace Hazel {

	enum class FramebufferTextureFormat_old
	{
		None = 0,

		// Color
		RGBA8,
		RED_INTEGER,

		// Depth/stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification_old
	{
		FramebufferTextureSpecification_old() = default;
		FramebufferTextureSpecification_old(FramebufferTextureFormat_old format)
			: TextureFormat(format) {}

		FramebufferTextureFormat_old TextureFormat = FramebufferTextureFormat_old::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecification_old
	{
		FramebufferAttachmentSpecification_old() = default;
		FramebufferAttachmentSpecification_old(std::initializer_list<FramebufferTextureSpecification_old> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification_old> Attachments;
	};

	struct FramebufferSpecification_old
	{
		uint32_t Width = 0, Height = 0;
		FramebufferAttachmentSpecification_old Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class Framebuffer_old
	{
	public:
		virtual ~Framebuffer_old() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		virtual const FramebufferSpecification_old& GetSpecification() const = 0;

		static Ref_old<Framebuffer_old> Create_old(const FramebufferSpecification_old& spec);
	};


}
