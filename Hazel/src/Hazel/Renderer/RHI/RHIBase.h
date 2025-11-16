#pragma once
#include "Hazel/Core/Base.h"
namespace Hazel {
#define MAX_QUEUE_CNT 2					//每个队列族的最大队列数目


	typedef std::shared_ptr<class DynamicRHI> DynamicRHIRef;
	typedef std::shared_ptr<class RHIResource> RHIResourceRef;
	typedef std::shared_ptr<class RHICommandContextImmediate> RHICommandContextImmediateRef;
	typedef std::shared_ptr<class RHIQueue> RHIQueueRef;
	typedef std::shared_ptr<class RHICommandListImmediate> RHICommandListImmediateRef;
	typedef std::shared_ptr<class RHISurface> RHISurfaceRef;
	typedef std::shared_ptr<class RHITexture> RHITextureRef;
	typedef std::shared_ptr<class RHIFence> RHIFenceRef;
	typedef std::shared_ptr<class RHISemaphore> RHISemaphoreRef;
	typedef std::shared_ptr<class RHICommandContext> RHICommandContextRef;
	typedef std::shared_ptr<class RHICommandPool> RHICommandPoolRef;
	typedef std::shared_ptr<class RHICommandList> RHICommandListRef;
	typedef std::shared_ptr<class RHISwapchain> RHISwapchainRef;

	enum API {
		Vulkan,
		OpenGL,
		DirectX,
		None
	};
	struct RHIConfig {
		API api = None;
		bool debug = false;
		bool enableRayTracing = false;
	};
	enum RHIResourceType : uint32_t
	{
		RHI_BUFFER = 0,
		RHI_TEXTURE,
		RHI_TEXTURE_VIEW,
		RHI_SAMPLER,
		RHI_SHADER,
		RHI_SHADER_BINDING_TABLE,
		RHI_TOP_LEVEL_ACCELERATION_STRUCTURE,
		RHI_BOTTOM_LEVEL_ACCELERATION_STRUCTURE,

		RHI_ROOT_SIGNATURE,
		RHI_DESCRIPTOR_SET,

		RHI_RENDER_PASS,
		RHI_GRAPHICS_PIPELINE,
		RHI_COMPUTE_PIPELINE,
		RHI_RAY_TRACING_PIPELINE,

		RHI_QUEUE,
		RHI_SURFACE,
		RHI_SWAPCHAIN,
		RHI_COMMAND_POOL,
		RHI_COMMAND_CONTEXT,
		RHI_COMMAND_CONTEXT_IMMEDIATE,
		RHI_FENCE,
		RHI_SEMAPHORE,

		RHI_RESOURCE_TYPE_MAX_CNT,	//
	};
	enum QueueType : uint32_t
	{
		QUEUE_TYPE_GRAPHICS = 0,
		QUEUE_TYPE_COMPUTE,
		QUEUE_TYPE_TRANSFER,

		QUEUE_TYPE_MAX_ENUM,
	};
	enum RHIFormat : uint32_t
	{
		FORMAT_UKNOWN = 0,

		FORMAT_R8_SRGB,
		FORMAT_R8G8_SRGB,
		FORMAT_R8G8B8_SRGB,
		FORMAT_R8G8B8A8_SRGB,
		FORMAT_B8G8R8A8_SRGB,

		FORMAT_R16_SFLOAT,
		FORMAT_R16G16_SFLOAT,
		FORMAT_R16G16B16_SFLOAT,
		FORMAT_R16G16B16A16_SFLOAT,
		FORMAT_R32_SFLOAT,
		FORMAT_R32G32_SFLOAT,
		FORMAT_R32G32B32_SFLOAT,
		FORMAT_R32G32B32A32_SFLOAT,

		FORMAT_R8_UNORM,
		FORMAT_R8G8_UNORM,
		FORMAT_R8G8B8_UNORM,
		FORMAT_R8G8B8A8_UNORM,
		FORMAT_R16_UNORM,
		FORMAT_R16G16_UNORM,
		FORMAT_R16G16B16_UNORM,
		FORMAT_R16G16B16A16_UNORM,

		FORMAT_R8_SNORM,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8B8_SNORM,
		FORMAT_R8G8B8A8_SNORM,
		FORMAT_R16_SNORM,
		FORMAT_R16G16_SNORM,
		FORMAT_R16G16B16_SNORM,
		FORMAT_R16G16B16A16_SNORM,

		FORMAT_R8_UINT,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8B8_UINT,
		FORMAT_R8G8B8A8_UINT,
		FORMAT_R16_UINT,
		FORMAT_R16G16_UINT,
		FORMAT_R16G16B16_UINT,
		FORMAT_R16G16B16A16_UINT,
		FORMAT_R32_UINT,
		FORMAT_R32G32_UINT,
		FORMAT_R32G32B32_UINT,
		FORMAT_R32G32B32A32_UINT,

		FORMAT_R8_SINT,
		FORMAT_R8G8_SINT,
		FORMAT_R8G8B8_SINT,
		FORMAT_R8G8B8A8_SINT,
		FORMAT_R16_SINT,
		FORMAT_R16G16_SINT,
		FORMAT_R16G16B16_SINT,
		FORMAT_R16G16B16A16_SINT,
		FORMAT_R32_SINT,
		FORMAT_R32G32_SINT,
		FORMAT_R32G32B32_SINT,
		FORMAT_R32G32B32A32_SINT,

		FORMAT_D32_SFLOAT,
		FORMAT_D32_SFLOAT_S8_UINT,
		FORMAT_D24_UNORM_S8_UINT,

		FORMAT_MAX_ENUM, 	//
	};
	enum MemoryUsage : uint32_t
	{
		MEMORY_USAGE_UNKNOWN = 0,
		MEMORY_USAGE_GPU_ONLY = 1,		// 仅GPU使用，在VRAM显存上分配，不可绑定
		MEMORY_USAGE_CPU_ONLY = 2,		// HOST_VISIBLE &&  HOST_COHERENT 及时同步，不需要flush到GPU，GPU可访问但是很慢
		MEMORY_USAGE_CPU_TO_GPU = 3,	// HOST_VISIBLE CPU端uncached，用于CPU端频繁进行数据写入，GPU端对数据进行读取
		MEMORY_USAGE_GPU_TO_CPU = 4,	// HOST_VISIBLE CPU端cached，用于被GPU写入且被CPU读取

		MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF,		//
	};
	enum ResourceTypeBits : uint32_t
	{
		RESOURCE_TYPE_NONE = 0x00000000,
		RESOURCE_TYPE_SAMPLER = 0x00000001,
		RESOURCE_TYPE_TEXTURE = 0x00000002,
		RESOURCE_TYPE_RW_TEXTURE = 0x00000004,
		RESOURCE_TYPE_TEXTURE_CUBE = 0x00000008,
		RESOURCE_TYPE_RENDER_TARGET = 0x00000010,
		RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER = 0x00000020,
		RESOURCE_TYPE_BUFFER = 0x00000040,
		RESOURCE_TYPE_RW_BUFFER = 0x00000080,
		RESOURCE_TYPE_UNIFORM_BUFFER = 0x00000100,
		RESOURCE_TYPE_VERTEX_BUFFER = 0x00000200,
		RESOURCE_TYPE_INDEX_BUFFER = 0x00000400,
		RESOURCE_TYPE_INDIRECT_BUFFER = 0x00000800,
		RESOURCE_TYPE_TEXEL_BUFFER = 0x00001000,
		RESOURCE_TYPE_RW_TEXEL_BUFFER = 0x00002000,
		RESOURCE_TYPE_RAY_TRACING = 0x00004000,

		RESOURCE_TYPE_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t ResourceType;
	enum TextureCreationFlagBits : uint32_t
	{
		TEXTURE_CREATION_NONE = 0x00000000,
		TEXTURE_CREATION_FORCE_2D = 0x00000001,
		TEXTURE_CREATION_FORCE_3D = 0x00000002,

		TEXTURE_CREATION_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t TextureCreationFlags;
	typedef struct RHIQueueInfo
	{
		QueueType type;
		uint32_t index;

	} RHIQueueInfo;

	typedef struct Extent2D
	{
		uint32_t    width;
		uint32_t    height;

		friend bool operator==(const Extent2D& a, const Extent2D& b)
		{
			return 	a.width == b.width &&
				a.height == b.height;
		}

		const uint32_t MipSize() const
		{
			return (uint32_t)(std::floor(std::log2(std::max(width, height)))) + 1;
		}

	} Extent2D;


	typedef struct Extent3D
	{
		uint32_t    width;
		uint32_t    height;
		uint32_t    depth;

		friend bool operator==(const Extent3D& a, const Extent3D& b)
		{
			return 	a.width == b.width &&
				a.height == b.height &&
				a.depth == b.depth;
		}

		const uint32_t MipSize() const
		{
			return (uint32_t)(std::floor(std::log2(std::max(width, std::max(height, depth))))) + 1;
		}

	} Extent3D;
	enum TextureAspectFlagBits : uint32_t
	{
		TEXTURE_ASPECT_NONE = 0x00000000,
		TEXTURE_ASPECT_COLOR = 0x00000001,
		TEXTURE_ASPECT_DEPTH = 0x00000002,
		TEXTURE_ASPECT_STENCIL = 0x00000004,

		TEXTURE_ASPECT_DEPTH_STENCIL = TEXTURE_ASPECT_DEPTH | TEXTURE_ASPECT_STENCIL,

		TEXTURE_ASPECT_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t TextureAspectFlags;
	typedef struct TextureSubresourceRange
	{
		TextureAspectFlags	  aspect = TEXTURE_ASPECT_NONE;
		uint32_t              baseMipLevel = 0;
		uint32_t              levelCount = 0;
		uint32_t              baseArrayLayer = 0;
		uint32_t              layerCount = 0;

		uint32_t			  __padding = 0;

		friend bool operator==(const TextureSubresourceRange& a, const TextureSubresourceRange& b)
		{
			return 	a.aspect == b.aspect &&
				a.baseMipLevel == b.baseMipLevel &&
				a.levelCount == b.levelCount &&
				a.baseArrayLayer == b.baseArrayLayer &&
				a.layerCount == b.layerCount;
		}

		bool IsDefault()
		{
			return 	aspect == TEXTURE_ASPECT_NONE &&
				baseMipLevel == 0 &&
				levelCount == 0 &&
				baseArrayLayer == 0 &&
				layerCount == 0;
		}

	} TextureSubresourceRange;

	typedef struct TextureSubresourceLayers
	{
		TextureAspectFlags	  aspect = TEXTURE_ASPECT_NONE;
		uint32_t              mipLevel = 0;
		uint32_t              baseArrayLayer = 0;
		uint32_t              layerCount = 0;

		friend bool operator==(const TextureSubresourceLayers& a, const TextureSubresourceLayers& b)
		{
			return 	a.aspect == b.aspect &&
				a.mipLevel == b.mipLevel &&
				a.baseArrayLayer == b.baseArrayLayer &&
				a.layerCount == b.layerCount;
		}

		bool IsDefault()
		{
			return 	aspect == TEXTURE_ASPECT_NONE &&
				mipLevel == 0 &&
				baseArrayLayer == 0 &&
				layerCount == 0;
		}

	} TextureSubresourceLayers;

	typedef struct RHISwapchainInfo
	{
		RHISurfaceRef surface;
		RHIQueueRef presentQueue;

		uint32_t imageCount;
		Extent2D extent;
		RHIFormat format;

	} RHISwapchainInfo;

	typedef struct RHITextureInfo
	{
		RHIFormat format;
		Extent3D extent;
		uint32_t arrayLayers = 1;
		uint32_t mipLevels = 1;

		MemoryUsage memoryUsage = MEMORY_USAGE_GPU_ONLY;
		ResourceType type = RESOURCE_TYPE_TEXTURE;

		TextureCreationFlags creationFlag = TEXTURE_CREATION_NONE;

		friend bool operator== (const RHITextureInfo& a, const RHITextureInfo& b)
		{
			return  a.format == b.format &&
				a.extent == b.extent &&
				a.arrayLayers == b.arrayLayers &&
				a.mipLevels == b.mipLevels &&
				a.memoryUsage == b.memoryUsage &&
				a.type == b.type &&
				a.creationFlag == b.creationFlag;
		}

	} RHITextureInfo;
	typedef struct RHICommandPoolInfo
	{
		RHIQueueRef queue;

	} RHICommandPoolInfo;
	typedef struct CommandListInfo
	{
		RHICommandPoolRef pool;
		RHICommandContextRef context;

		bool byPass = true; 		//是否立即录制

	} CommandListInfo;
	typedef struct CommandListImmediateInfo
	{
		RHICommandContextImmediateRef context;

	} CommandListImmediateInfo;


	static bool IsDepthStencilFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	static bool IsDepthFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT:
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	static bool IsStencilFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	static bool IsColorFormat(RHIFormat format)
	{
		return !IsDepthFormat(format) && !IsStencilFormat(format);
	}

	static bool IsRWFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT:
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
		case FORMAT_R8_SRGB:
		case FORMAT_R8G8_SRGB:
		case FORMAT_R8G8B8_SRGB:
		case FORMAT_R8G8B8A8_SRGB:
		case FORMAT_B8G8R8A8_SRGB:
			return false;
		default:
			return true;
		}
	}
}