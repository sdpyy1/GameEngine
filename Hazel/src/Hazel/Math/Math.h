#pragma once

#include <glm/glm.hpp>

namespace Hazel::Math {
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);
	// 1. 处理整数标量 ÷ 整数标量（如 uint32_t ÷ uint32_t）
		// 约束：T 必须是整数类型（排除向量类型）
	template<typename T,
		std::enable_if_t<std::is_integral_v<T>, int> = 0>
	inline static T DivideAndRoundUp(T dividend, T divisor) {
		return (dividend + divisor - 1) / divisor;
	}

	// 2. 处理 glm::uvec2 向量 ÷ 整数标量（如 uvec2(800,600) ÷ 8）
	// 约束：第一个参数必须是 glm::uvec2，第二个参数必须是整数类型
	template<typename DivisorT,
		std::enable_if_t<std::is_integral_v<DivisorT>, int> = 0>
	inline static glm::uvec2 DivideAndRoundUp(glm::uvec2 dividend, DivisorT divisor) {
		// 转换除数为 uint32_t（与 glm::uvec2 的分量类型匹配）
		uint32_t div = static_cast<uint32_t>(divisor);
		return {
			DivideAndRoundUp(dividend.x, div),  // 调用第一个重载处理 x 分量
			DivideAndRoundUp(dividend.y, div)   // 调用第一个重载处理 y 分量
		};
	}
}
