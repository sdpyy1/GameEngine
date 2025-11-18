#include "hzpch.h"
#include "RDGBuilder.h"

namespace GameEngine { 

    RDGResourceHandle RDGBuilder::CreateTexture(const RDGTextureDesc& Desc, const std::string& Name)
    {
        RDGTextureRef Tex = std::make_shared<RDGTexture>(Desc, Name);

        FRDGResourceEntry Entry;
        Entry.Handle.Index = static_cast<uint32_t>(m_ResourceTable.size());
        Entry.Resource = Tex;

        m_ResourceTable.push_back(Entry);
        m_RDGContext.registerTexture(Name, Tex);
        return Entry.Handle;
    }
    RDGResourceHandle RDGBuilder::CreateBuffer(const RDGBufferDesc& Desc, const std::string& Name)
    {
        RDGBufferRef Buf = std::make_shared<RDGBuffer>(Desc, Name);

        FRDGResourceEntry Entry;
        Entry.Handle.Index = static_cast<uint32_t>(m_ResourceTable.size());
        Entry.Resource = Buf;

        m_ResourceTable.push_back(Entry);
        m_RDGContext.registerBuffer(Name, Buf);

        return Entry.Handle;
    }

    RDGRenderPassBuilder RDGBuilder::AddPass(const std::string& Name)
	{
        RDGPassRef Pass = std::make_shared<RDGPass>(Name);
        m_PassList.push_back(Pass);
        m_RDGContext.registerPass(Name, Pass);

        return RDGRenderPassBuilder(this, Pass);
	}

	void RDGBuilder::Execute()
	{
        for (auto& Pass : m_PassList)
        {
            LOG_INFO("RDG Pass: {0}", Pass->Name);
            Pass->Execute();
        }
	}

	RDGRenderPassBuilder::RDGRenderPassBuilder(RDGBuilder* builder, RDGPassRef pass): m_Builder(builder), m_Pass(pass)
	{

	}

}