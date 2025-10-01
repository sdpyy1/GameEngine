#pragma once

#include "Hazel/Renderer/Texture.h"

#include <glad/glad.h>

namespace Hazel {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const TextureSpecification& specification);
		OpenGLTexture2D(const std::string& path);
		virtual ~OpenGLTexture2D();

		virtual const TextureSpecification& GetSpecification() const  { return m_Specification; }

		virtual uint32_t GetWidth() const override { return m_Width;  }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const  { return m_RendererID; }

		const std::filesystem::path& GetPath_old() const  { return m_Path; }
		
		virtual void SetData(void* data, uint32_t size) ;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool IsLoaded() const  { return m_IsLoaded; }

		virtual bool operator==(const Texture& other) const 
		{
			//return m_RendererID == other.GetRendererID();
			return m_RendererID == 1; // Ï¹¸Ä
		}
	private:
		TextureSpecification m_Specification;

		const std::filesystem::path m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

}
