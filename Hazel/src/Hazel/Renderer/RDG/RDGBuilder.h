#pragma once
#include "RDGResource.h"
#include "RDGPass.h"
namespace GameEngine {
	struct RDGContext {
		std::unordered_map<std::string, RDGPassRef> passes;
		std::unordered_map<std::string,RDGTextureRef> textures;
        std::unordered_map<std::string,RDGBufferRef> buffers;
		void registerPass(const std::string& name, RDGPassRef pass) { passes[name] = pass; }
		void registerTexture(const std::string& name, RDGTextureRef texture) { textures[name] = texture; }
        void registerBuffer(const std::string& name, RDGBufferRef buffer) { buffers[name] = buffer; }
        RDGTextureRef getTexture(const std::string& name) { return textures[name]; }
        RDGBufferRef getBuffer(const std::string& name) { return buffers[name]; }
        RDGPassRef getPass(const std::string& name) { return passes[name]; }
	};
	class RDGRenderPassBuilder;
	class RDGTextureBuilder;
	class RDGBufferBuilder;
	class RDGBuilder
	{
	public:
		RDGResourceHandle RDGBuilder::CreateTexture(const RHITextureInfo& Desc, const std::string& Name);
		RDGResourceHandle CreateBuffer(const RHIBufferInfo& Desc, const std::string& Name);
		RDGTextureBuilder CreateTexture(const std::string& name);
		RDGBufferBuilder CreateBuffer(const std::string& name);
		RDGRenderPassBuilder AddPass(const std::string& Name);

		RDGTextureRef GetTexture(const std::string& Name){return m_RDGContext.getTexture(Name);}
        RDGBufferRef GetBuffer(const std::string& Name){return m_RDGContext.getBuffer(Name);}
        RDGPassRef GetPass(const std::string& Name){return m_RDGContext.getPass(Name);}
		
		
		
		void Execute();
	private:
		struct FRDGResourceEntry
		{
			RDGResourceHandle Handle;
			RDGResourceRef Resource = nullptr; 
		};
		std::vector<FRDGResourceEntry> m_ResourceTable;
		std::vector<RDGPassRef> m_PassList;
		RDGContext m_RDGContext;
	};

	class RDGRenderPassBuilder {
	public:
		RDGRenderPassBuilder(RDGBuilder* builder, RDGPassRef pass);
		RDGTextureRef ReadTexture(const std::string& name);
		RDGTextureRef WriteTexture(const std::string& name);
		RDGBufferRef ReadBuffer(const std::string& name);
		RDGBufferRef WriteBuffer(const std::string& name);
	private:
        RDGBuilder* m_Builder;
        RDGPassRef m_Pass;

	};





	class RDGTextureBuilder {
	public:
		RDGTextureBuilder(RDGBuilder* builder, RDGTextureRef texture) :m_Builder(builder), m_Texture(texture) {};

	private:
		RDGBuilder* m_Builder;
		RDGTextureRef m_Texture;
	};


	class RDGBufferBuilder {
	public:
		RDGBufferBuilder(RDGBuilder* builder, RDGBufferRef buffer) :m_Builder(builder),m_Buffer(buffer){};
	private:
		RDGBuilder* m_Builder;
		RDGBufferRef m_Buffer;
	};


}

