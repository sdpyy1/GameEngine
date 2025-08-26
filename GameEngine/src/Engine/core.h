#pragma once

// ENGINE_API 用来表示API是引擎API，但是如果在构建DLL，他就表示要导出，如果在引用DLL，它表示要导入
#ifdef ENGINE_PLATFORM_WINDOWS
	#ifdef ENGINE_BUILD_DLL
		#define ENGINE_API __declspec(dllexport)
	#else
		#define ENGINE_API __declspec(dllimport)
	#endif
#else
	#error Engine only supports Windows!
#endif