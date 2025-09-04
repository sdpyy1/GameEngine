#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
namespace Hazel {
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_Projection(projection) {
		}

		virtual ~Camera() = default;



		const glm::mat4& GetProjection() const { return m_Projection; }

	protected:
		glm::mat4 m_Projection = glm::mat4(1.0f);
	};

}