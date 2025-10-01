#include "hzpch.h"
#include "Hazel/Core/Thread.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <Windows.h>

namespace Hazel {

	Thread::Thread(const std::string& name)
		: m_Name(name)
	{
	}

	void Thread::SetName(const std::string& name)
	{
		HANDLE threadHandle = m_Thread.native_handle();

		std::wstring wName(name.begin(), name.end());
		SetThreadDescription(threadHandle, wName.c_str());
		SetThreadAffinityMask(threadHandle, 8); // �����׺�������Ϊ8����ʾ���̰߳󶨵���4��CPU���ģ���0��ʼ������ ���SetName�޹�
	}

	void Thread::Join()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	ThreadSignal::ThreadSignal(const std::string& name, bool manualReset)
	{
		std::wstring str(name.begin(), name.end());
		m_SignalHandle = CreateEvent(NULL, (BOOL)manualReset, FALSE, str.c_str());
	}

	void ThreadSignal::Wait()
	{
		WaitForSingleObject(m_SignalHandle, INFINITE); // ����ֱ���¼�����
	}

	void ThreadSignal::Signal()
	{
		SetEvent(m_SignalHandle); // �����¼������ѵȴ����߳�
	}

	void ThreadSignal::Reset()
	{
		ResetEvent(m_SignalHandle);
	}

	std::thread::id Thread::GetID() const
	{
		return m_Thread.get_id();
	}

}
