#include "hzpch.h"
#include "Camera.h"

namespace Hazel {
	Camera::Camera(const glm::mat4& projection, const glm::mat4& unReversedProjection)
		: m_ProjectionMatrix(projection), m_UnReversedProjectionMatrix(unReversedProjection)
	{
	}

	Camera::Camera(const float degFov, const float width, const float height, const float nearP, const float farP)
		: m_ProjectionMatrix(glm::perspectiveFov(glm::radians(degFov), width, height, nearP,farP)), m_UnReversedProjectionMatrix(glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP))
	{
	}
}
