#pragma once

// ENGINE_API ������ʾAPI������API����������ڹ���DLL�����ͱ�ʾҪ���������������DLL������ʾҪ����
#ifdef ENGINE_PLATFORM_WINDOWS
	#ifdef ENGINE_BUILD_DLL
		#define ENGINE_API __declspec(dllexport)
	#else
		#define ENGINE_API __declspec(dllimport)
	#endif
#else
	#error Engine only supports Windows!
#endif