#include "hzpch.h"
#include "RenderCommandQueue.h"
#include "Hazel/Core/RenderThread.h"
#include "Hazel/Core/Application.h"

#define HZ_RENDER_TRACE(...) HZ_CORE_TRACE(__VA_ARGS__)

namespace Hazel {

	RenderCommandQueue::RenderCommandQueue()
	{
		m_CommandBuffer = new uint8_t[10 * 1024 * 1024]; // 10mb buffer
		m_CommandBufferPtr = m_CommandBuffer;
		memset(m_CommandBuffer, 0, 10 * 1024 * 1024); // ���0
	}

	RenderCommandQueue::~RenderCommandQueue()
	{
		delete[] m_CommandBuffer;
	}

	// �������������
	void* RenderCommandQueue::Allocate(RenderCommandFn fn, uint32_t size)
	{
		// NOTE(Yan): for debugging
		// HZ_CORE_VERIFY(!RenderThread::IsCurrentThreadRT());

		// TODO: alignment
		*(RenderCommandFn*)m_CommandBufferPtr = fn;
		m_CommandBufferPtr += sizeof(RenderCommandFn);

		*(uint32_t*)m_CommandBufferPtr = size;
		m_CommandBufferPtr += sizeof(uint32_t);

		void* memory = m_CommandBufferPtr;
		m_CommandBufferPtr += size;

		m_CommandCount++;
		return memory;
	}

	void RenderCommandQueue::Execute()
	{
		//HZ_RENDER_TRACE("RenderCommandQueue::Execute -- {0} commands, {1} bytes", m_CommandCount, (m_CommandBufferPtr - m_CommandBuffer));
		byte* buffer = m_CommandBuffer;
		if (Application::Get().isRunning() && !Application::Get().isMinimized()) {  // ��Ϊ��ʹ��С���¼�������������л�����һ֡����Ҫ���������ڴ�С�Ѿ���0�ˣ����Bug
			for (uint32_t i = 0; i < m_CommandCount; i++)
			{
				RenderCommandFn function = *(RenderCommandFn*)buffer;
				buffer += sizeof(RenderCommandFn);

				uint32_t size = *(uint32_t*)buffer;
				buffer += sizeof(uint32_t);
				function(buffer);
				buffer += size;
			}
		}


		m_CommandBufferPtr = m_CommandBuffer;
		m_CommandCount = 0;
	}

}
