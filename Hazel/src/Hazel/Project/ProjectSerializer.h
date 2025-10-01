#pragma once

#include "Project.h"

namespace Hazel {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref_old<Project> project);

		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
	private:
		Ref_old<Project> m_Project;
	};

}
