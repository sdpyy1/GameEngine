#include "pch.h"
#include "RendererAPI.h"
#include <glm/glm.hpp>


#include <glad/glad.h>

namespace Engine {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

}