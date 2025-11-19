#include "hzpch.h"
#include "VulkanRHI.h"
#include "VulkanRHICache.h"
namespace GameEngine {
    VkRenderPassCache::CachedRenderPass VkRenderPassCache::Allocate(const VulkanUtil::VulkanRenderPassAttachments& info)
    {
        VkRenderPassCache::CachedRenderPass ret;

        // 查找缓存
        auto iter = cachedPasses.find(info);
        if (iter != cachedPasses.end())
        {
            // LOG_DEBUG("VkRenderPass found in cache.");
            return iter->second;
        }

        LOG_WARN("VkRenderPass not found in cache, creating new.");

        // C++17 传统初始化
        ret.pass = VULKAN_RHI->CreateVkRenderPass(info);
        cachedPasses[info] = ret;

        return ret;
    }

    void VkRenderPassCache::Clear()
    {
        for (auto iter : cachedPasses)
        {
            vkDestroyRenderPass(VULKAN_DEVICE, iter.second.pass, nullptr);
        }
        cachedPasses.clear();
    }

    VkFramebufferCache::CachedFramebuffer VkFramebufferCache::Allocate(const VkFramebufferCreateInfo& info)
    {
        VkFramebufferCache::CachedFramebuffer ret;

        auto iter = cachedFramebuffers.find(info);
        if (iter != cachedFramebuffers.end())
        {
            // LOG_DEBUG("VkFramebuffer found in cache.");
            return iter->second;
        }

        LOG_WARN("VkFramebuffer not found in cache, creating new.");
        ret.frameBuffer = VULKAN_RHI->CreateVkFramebuffer(info);

        cachedFramebuffers[info] = ret;

        return ret;
    }

    void VkFramebufferCache::Clear()
    {
        for (auto iter : cachedFramebuffers)
        {
            vkDestroyFramebuffer(VULKAN_DEVICE, iter.second.frameBuffer, nullptr);
        }
        cachedFramebuffers.clear();
    }
}