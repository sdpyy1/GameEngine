#pragma once
#include "Hazel/Utils/IndexAllocator.h"
#include <Hazel/Renderer/RHI/RHI.h>
#include <Hazel/Renderer/RHI/RHIResource.h>
#include "Hazel/Core/Definations.h"
namespace GameEngine {
    // 使用Bindless的资源
    enum BindlessSlot
    {
        BINDLESS_SLOT_POSITION = 0,
        BINDLESS_SLOT_NORMAL,
        BINDLESS_SLOT_TANGENT,
        BINDLESS_SLOT_TEXCOORD,
        BINDLESS_SLOT_COLOR,
        BINDLESS_SLOT_BONE_INDEX,
        BINDLESS_SLOT_BONE_WEIGHT,
        BINDLESS_SLOT_ANIMATION,
        BINDLESS_SLOT_INDEX,

        BINDLESS_SLOT_SAMPLER,
        BINDLESS_SLOT_TEXTURE_1D,
        BINDLESS_SLOT_TEXTURE_1D_ARRAY,
        BINDLESS_SLOT_TEXTURE_2D,
        BINDLESS_SLOT_TEXTURE_2D_ARRAY,
        BINDLESS_SLOT_TEXTURE_CUBE,
        BINDLESS_SLOT_TEXTURE_3D,

        BINDLESS_SLOT_MAX_ENUM,     //
    };
    enum GlobalResourceBindingID {
        GLORBAL_RESOURCE_BINDING_SETTING = 0,
        GLORBAL_RESOURCE_BINDING_CAMERA,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXUTE_2D,
    };
    struct GlobalResourcesPreFrame
    {
        RHIDescriptorSetRef descriptorSet;
    };
	class RenderResourceManager
	{
		public:
            RenderResourceManager();
            ~RenderResourceManager() {};
            void InitGlobalResources();
	private:


    

       std::array<GlobalResourcesPreFrame, FRAMES_IN_FLIGHT> GlobalResources;

		std::array<IndexAllocator, BINDLESS_SLOT_MAX_ENUM> bindlessIDAlloctor; // 每种资源一个ID分配器
        RHIRootSignatureRef GlobalResourceRootSignature; 

	};
}

