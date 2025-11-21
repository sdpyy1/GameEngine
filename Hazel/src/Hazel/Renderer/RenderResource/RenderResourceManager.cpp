#include "hzpch.h"
#include "RenderResourceManager.h"
#include "Hazel/Core/Application.h"

#define MAX_BINDLESS_RESOURCE_SIZE 10240	        //bindless 单个binding的最大描述符数目

namespace GameEngine {
	RenderResourceManager::RenderResourceManager()
	{
		for (auto& alloctor : bindlessIDAlloctor) alloctor = IndexAllocator(MAX_BINDLESS_RESOURCE_SIZE);
		InitGlobalResources();
	}

	void RenderResourceManager::InitGlobalResources()
	{
		// 创建一个全局的资源描述符集来存储各种全局资源
		RHIRootSignatureInfo info = {};
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_SETTING, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_CAMERA, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXUTE_2D, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });
		GlobalResourceRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		for (auto& resource : GlobalResources) resource.descriptorSet = GlobalResourceRootSignature->CreateDescriptorSet(0);



		for (auto& resource : GlobalResources) {

			RHIDescriptorUpdateInfo updateInfo = {};
			updateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
            updateInfo.buffer = resource.buffer;  // TODO:得先创建这个资源
			updateInfo.index = 0;
			updateInfo.binding = GLORBAL_RESOURCE_BINDING_SETTING;

			resource.descriptorSet->UpdateDescriptor(updateInfo);



		}

	}

}