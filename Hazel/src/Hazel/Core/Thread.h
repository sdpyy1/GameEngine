#pragma once
#include <string>
#include <thread>

namespace Hazel {

	class Thread
	{
	public:
		Thread(const std::string& name);

		// �߳�ִ��
		template<typename Fn, typename... Args>
		void Dispatch(Fn&& func, Args&&... args)
		{
			m_Thread = std::thread(func, std::forward<Args>(args)...); // ���л��������߳�ִ�д���ĺ���
			SetName(m_Name);
			HZ_CORE_INFO("Thread [{0}] Dispatch and Run!", m_Name);
		}

		void SetName(const std::string& name);

		// �����ȴ��߳̽���
		void Join();

		std::thread::id GetID() const;
	private:
		std::string m_Name;
		std::thread m_Thread;
	};

	// �߳��źţ������̼߳�ͬ��
	class ThreadSignal
	{
	public:
		// manualReset: true��ʾ�ֶ����ã�false��ʾ�Զ����� false��ʾһ�η���һ��
		ThreadSignal(const std::string& name, bool manualReset = false);

		void Wait();
		void Signal();
		void Reset();
	private:
		void* m_SignalHandle = nullptr;
	};

}
