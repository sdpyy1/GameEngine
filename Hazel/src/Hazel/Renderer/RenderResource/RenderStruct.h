#pragma once
#include <glm/ext/matrix_float4x4.hpp>
namespace GameEngine {
	namespace V2 {
		struct CameraData {
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
			float Width;
			float Height;
			float Near;
			float Far;
			glm::vec3 Position;
			float padding;
			glm::mat4 InverseViewProj;
		};
	}
}