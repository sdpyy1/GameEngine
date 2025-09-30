#pragma once

#include "Hazel/Core/Base.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Hazel {

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

// Core log macros
#define HZ_CORE_TRACE(...)    ::Hazel::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HZ_CORE_INFO(...)     ::Hazel::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HZ_CORE_WARN(...)     ::Hazel::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HZ_CORE_ERROR(...)    ::Hazel::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HZ_CORE_CRITICAL(...) ::Hazel::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define HZ_TRACE(...)         ::Hazel::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HZ_INFO(...)          ::Hazel::Log::GetClientLogger()->info(__VA_ARGS__)
#define HZ_WARN(...)          ::Hazel::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HZ_ERROR(...)         ::Hazel::Log::GetClientLogger()->error(__VA_ARGS__)
#define HZ_CRITICAL(...)      ::Hazel::Log::GetClientLogger()->critical(__VA_ARGS__)

// 临时修改：忽略tag参数，复用无标签宏的实现
#define HZ_CORE_TRACE_TAG(tag, ...) HZ_CORE_TRACE(__VA_ARGS__)
#define HZ_CORE_INFO_TAG(tag, ...)  HZ_CORE_INFO(__VA_ARGS__)
#define HZ_CORE_WARN_TAG(tag, ...)  HZ_CORE_WARN(__VA_ARGS__)
#define HZ_CORE_ERROR_TAG(tag, ...) HZ_CORE_ERROR(__VA_ARGS__)
#define HZ_CORE_FATAL_TAG(tag, ...) HZ_CORE_CRITICAL(__VA_ARGS__)  // 注意原宏中CORE_CRITICAL对应Fatal级别

// Client logging
#define HZ_TRACE_TAG(tag, ...) HZ_TRACE(__VA_ARGS__)
#define HZ_INFO_TAG(tag, ...)  HZ_INFO(__VA_ARGS__)
#define HZ_WARN_TAG(tag, ...)  HZ_WARN(__VA_ARGS__)
#define HZ_ERROR_TAG(tag, ...) HZ_ERROR(__VA_ARGS__)
#define HZ_FATAL_TAG(tag, ...) HZ_CRITICAL(__VA_ARGS__)  // 注意原宏中CLIENT_CRITICAL对应Fatal级别
