#include "hzpch.h"
#include "RenderResourceManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Scene/SceneManager.h"
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
		m_GlobalResourceRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		for (auto& resource : m_GlobalResources) resource.descriptorSet = m_GlobalResourceRootSignature->CreateDescriptorSet(0);



		for (auto& resource : m_GlobalResources) {

			RHIDescriptorUpdateInfo updateInfo = {};
			updateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
            updateInfo.buffer = resource.cameraDataBuffer.GetRHIBuffer();
			updateInfo.index = 0;
			updateInfo.binding = GLORBAL_RESOURCE_BINDING_SETTING;
			resource.descriptorSet->UpdateDescriptor(updateInfo);

		}

	}

	void RenderResourceManager::SetCameraInfo()
	{
		V2::CameraData tmpdata;
		EditorCamera& camera = m_SceneInfoFromScene.camera;
		tmpdata.view = camera.GetViewMatrix();
		tmpdata.proj = camera.GetProjectionMatrix();
		tmpdata.proj[1][1] *= -1;
		tmpdata.viewproj = camera.GetViewProjection();
		tmpdata.Width = camera.GetViewportWidth();
		tmpdata.Height = camera.GetViewportWidth();
		tmpdata.Near = camera.GetNearClip();
		tmpdata.Far = camera.GetFarClip();
		tmpdata.Position = camera.GetPosition();
		tmpdata.padding = 1.f;
		tmpdata.InverseViewProj = glm::inverse(camera.GetViewProjection());
		m_GlobalResources[APP_FRAMEINDEX].cameraDataBuffer.SetData(tmpdata);
	}

	void RenderResourceManager::Tick()
	{
		m_SceneInfoFromScene = APP_SCENEMANAGER->GetSceneInfo();
		SetCameraInfo();
	}

}