#pragma once
namespace Engine {
	class GraphicsContext
	{
	public:
		// �������API��γ�ʼ�������ĺ���ν���������
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
	};
};