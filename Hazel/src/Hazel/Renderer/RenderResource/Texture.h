#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
namespace GameEngine {
	namespace V2 {

		enum TextureType {
			TEXTURE_TYPE_2D,
			TEXTURE_TYPE_2D_ARRAY,
			TEXTURE_TYPE_CUBE,
			TEXTURE_TYPE_3D
		};
		struct TextureSpce {
			std::filesystem::path path;
            TextureType type = TEXTURE_TYPE_2D;
			Extent3D extent = {1,1,0};
            RHIFormat format = FORMAT_R8G8B8A8_SRGB;   // ×Ô¶¯Ù¤Âí
			bool srgb = true;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            bool generateMipmap = false;
			RHITextureRef texture;
			RHITextureViewRef textureView;
		};
		class Texture {
		public:
			Texture(TextureSpce& spec);
			void LoadFromFile();

		private:
			TextureSpce m_Spec;
		};

		
	}

}

