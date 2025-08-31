#pragma once
// #pragma once只是防止一个cpp文件多次编译一个头文件，而pcf可以防止一个头文件被多个cpp文件编译
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <ostream>
#include <memory>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Engine/Log.h"


#ifdef ENGINE_PLATFORM_WINDOWS
	#include <Windows.h>
#endif