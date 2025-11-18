#pragma once
#include "Hazel/Core/Base.h"
namespace GameEngine {
#define MAX_QUEUE_CNT 2					//每个队列族的最大队列数目
#define MAX_SHADER_IN_OUT_VARIABLES 8	//允许着色器最大的输入和输出变量数目

#define RHI_DYNAMICRHI DynamicRHI::Get()

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
	typedef std::shared_ptr<class RHITextureView> RHITextureViewRef;
	typedef std::shared_ptr<class RHISampler> RHISamplerRef;
	typedef std::shared_ptr<class RHIShader> RHIShaderRef;

	enum API {
		API_Vulkan,
		OpenGL,
		DirectX,
		NoneAPI
	};
	struct RHIConfig {
		API api = NoneAPI;
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
	enum TextureViewType : uint32_t
	{
		VIEW_TYPE_UNDEFINED = 0,
		VIEW_TYPE_1D,
		VIEW_TYPE_2D,
		VIEW_TYPE_3D,
		VIEW_TYPE_CUBE,
		VIEW_TYPE_1D_ARRAY,
		VIEW_TYPE_2D_ARRAY,
		VIEW_TYPE_CUBE_ARRAY,

		VIEW_TYPE_MAX_ENUM,		//	
	};
	enum FilterType : uint32_t
	{
		FILTER_TYPE_NEAREST = 0,
		FILTER_TYPE_LINEAR,

		FILTER_TYPE_MAX_ENUM,	//
	};

	enum MipMapMode : uint32_t
	{
		MIPMAP_MODE_NEAREST = 0,
		MIPMAP_MODE_LINEAR,

		MIPMAP_MODE_MAX_ENUM_BIT,	//
	};
	enum AddressMode : uint32_t
	{
		ADDRESS_MODE_MIRROR,
		ADDRESS_MODE_REPEAT,
		ADDRESS_MODE_CLAMP_TO_EDGE,
		ADDRESS_MODE_CLAMP_TO_BORDER,

		ADDRESS_MODE_MAX_ENUM,	//
	};
	enum CompareFunction : uint32_t
	{
		COMPARE_FUNCTION_LESS = 0,
		COMPARE_FUNCTION_LESS_EQUAL,
		COMPARE_FUNCTION_GREATER,
		COMPARE_FUNCTION_GREATER_EQUAL,
		COMPARE_FUNCTION_EQUAL,
		COMPARE_FUNCTION_NOT_EQUAL,
		COMPARE_FUNCTION_NEVER,
		COMPARE_FUNCTION_ALWAYS,

		COMPARE_FUNCTION_MAX_ENUM,   //
	};
	enum SamplerReductionMode : uint32_t
	{
		SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE = 0,
		SAMPLER_REDUCTION_MODE_MIN,
		SAMPLER_REDUCTION_MODE_MAX,

		SAMPLER_REDUCTION_MODE_MAX_ENUM,   //
	};
	typedef struct RHISamplerInfo
	{
		FilterType minFilter = FILTER_TYPE_LINEAR;
		FilterType magFilter = FILTER_TYPE_LINEAR;
		MipMapMode mipmapMode = MIPMAP_MODE_LINEAR;
		AddressMode addressModeU = ADDRESS_MODE_REPEAT;
		AddressMode addressModeV = ADDRESS_MODE_REPEAT;
		AddressMode addressModeW = ADDRESS_MODE_REPEAT;
		CompareFunction compareFunction = COMPARE_FUNCTION_NEVER;
		SamplerReductionMode reductionMode = SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;

		float mipLodBias = 0.0f;
		float maxAnisotropy = 0.0f;

	} RHISamplerInfo;
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
	typedef struct RHITextureViewInfo
	{
		RHITextureRef texture;
		RHIFormat format = FORMAT_UKNOWN;			// 此时取texture的format
		TextureViewType viewType = VIEW_TYPE_2D;

		TextureSubresourceRange subresource;	// 此时取texture的默认range

		friend bool operator== (const RHITextureViewInfo& a, const RHITextureViewInfo& b)
		{
			return  a.texture.get() == b.texture.get() &&
				a.format == b.format &&
				a.viewType == b.viewType &&
				a.subresource == b.subresource;
		}

	} RHITextureViewInfo;

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

	
	enum ShaderFrequencyBits : uint32_t
	{
		SHADER_FREQUENCY_COMPUTE = 0x00000001,
		SHADER_FREQUENCY_VERTEX = 0x00000002,
		SHADER_FREQUENCY_FRAGMENT = 0x00000004,
		SHADER_FREQUENCY_GEOMETRY = 0x00000008,
		SHADER_FREQUENCY_RAY_GEN = 0x00000010,
		SHADER_FREQUENCY_CLOSEST_HIT = 0x00000020,
		SHADER_FREQUENCY_RAY_MISS = 0x00000040,
		SHADER_FREQUENCY_INTERSECTION = 0x00000080,
		SHADER_FREQUENCY_ANY_HIT = 0x00000100,
		SHADER_FREQUENCY_MESH = 0x00000200,

		SHADER_FREQUENCY_GRAPHICS = SHADER_FREQUENCY_VERTEX |
		SHADER_FREQUENCY_FRAGMENT |
		SHADER_FREQUENCY_GEOMETRY |
		SHADER_FREQUENCY_MESH,
		SHADER_FREQUENCY_RAY_TRACING = SHADER_FREQUENCY_RAY_GEN |
		SHADER_FREQUENCY_CLOSEST_HIT |
		SHADER_FREQUENCY_RAY_MISS |
		SHADER_FREQUENCY_INTERSECTION |
		SHADER_FREQUENCY_ANY_HIT,
		SHADER_FREQUENCY_ALL = SHADER_FREQUENCY_GRAPHICS |
		SHADER_FREQUENCY_COMPUTE |
		SHADER_FREQUENCY_RAY_TRACING,

		SHADER_FREQUENCY_MAX_ENUM = 0x7FFFFFFF, //
	};
	typedef uint32_t ShaderFrequency;
	enum RHIResourceState : uint32_t	
	{
		RESOURCE_STATE_UNDEFINED = 0,
		RESOURCE_STATE_COMMON,
		RESOURCE_STATE_TRANSFER_SRC,
		RESOURCE_STATE_TRANSFER_DST,
		RESOURCE_STATE_VERTEX_BUFFER,
		RESOURCE_STATE_INDEX_BUFFER,
		RESOURCE_STATE_COLOR_ATTACHMENT,
		RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT,
		RESOURCE_STATE_UNORDERED_ACCESS,
		RESOURCE_STATE_SHADER_RESOURCE,
		RESOURCE_STATE_INDIRECT_ARGUMENT,
		RESOURCE_STATE_PRESENT,
		RESOURCE_STATE_ACCELERATION_STRUCTURE,

		RESOURCE_STATE_MAX_ENUM,	//
	};
	typedef struct ShaderResourceEntry
	{
		// std::string name;

		uint32_t set = 0;
		uint32_t binding = 0;
		uint32_t size = 1;
		ShaderFrequency frequency = SHADER_FREQUENCY_ALL;

		ResourceType type = RESOURCE_TYPE_NONE;
		// TextureViewType textureViewType = VIEW_TYPE_UNDEFINED;	// 只有反射会填的信息，创建描述符绑定并不需要

		friend bool operator== (const ShaderResourceEntry& a, const ShaderResourceEntry& b)
		{
			return  a.set == b.set &&
				a.binding == b.binding &&
				a.size == b.size &&
				a.frequency == b.frequency &&
				a.type == b.type;
			// a.textureViewType == b.textureViewType;
		}

	} ShaderResourceEntry;
	typedef struct ShaderReflectInfo
	{
		std::string name;

		ShaderFrequency frequency;
		std::vector<ShaderResourceEntry> resources;
		std::unordered_set<std::string> definedSymbols;
		std::array<RHIFormat, MAX_SHADER_IN_OUT_VARIABLES> inputVariables = {};
		std::array<RHIFormat, MAX_SHADER_IN_OUT_VARIABLES> outputVariables = {};
		uint32_t localSizeX = 0;
		uint32_t localSizeY = 0;
		uint32_t localSizeZ = 0;

		bool DefinedSymbol(std::string symbol) const { return definedSymbols.find(symbol) != definedSymbols.end(); }

	} ShaderReflectInfo;
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

	typedef struct RHIShaderInfo
	{
		std::string entry = "main";

		ShaderFrequency frequency;
		std::vector<uint8_t> code;
	} RHIShaderInfo;

	typedef struct RHITextureBarrier
	{
		RHITextureRef texture;
		RHIResourceState srcState;
		RHIResourceState dstState;

		TextureSubresourceRange subresource = {};	// 此时取texture的默认range

	} RHITextureBarrier;



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