#pragma once
#include "string"
#include "Hazel/Renderer/RDG/RDGBuilder.h"
namespace GameEngine {
	enum PassType
	{


		PASS_TYPE_MAX_CNT,	//
	};

	enum MeshPassType
	{
		

		MESH_PASS_TYPE_MAX_CNT,	//
	};
	class RenderPass {
	public:
		RenderPass() = default;
		~RenderPass() {};

		virtual void Init() {};
		virtual void Build(RDGBuilder& builder) {};

		virtual std::string GetName() { return "Unknown"; }

		virtual PassType GetType() = 0;

		bool IsEnabled() { return enable; }
		void SetEnable(bool enable) { this->enable = enable; }
	private:
		bool enable = true;
	};
}