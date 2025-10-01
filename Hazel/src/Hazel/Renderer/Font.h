#pragma once

#include <filesystem>

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/Texture.h"

namespace Hazel {

	struct MSDFData;

	class Font
	{
	public:
		Font(const std::filesystem::path& font);
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		Ref_old<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

		static Ref_old<Font> GetDefault();
	private:
		MSDFData* m_Data;
		Ref_old<Texture2D> m_AtlasTexture;
	};

}
