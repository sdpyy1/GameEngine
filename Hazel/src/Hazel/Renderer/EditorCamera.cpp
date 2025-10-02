#include "hzpch.h"
#include "EditorCamera.h"

#include "Hazel/Core/Input.h"
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"

#include <glfw/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <Hazel.h>

namespace Hazel {

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
	{
		UpdateView();
	}

	void EditorCamera::UpdateProjection()
	{
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdateView()
	{
		// m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
		m_Position = CalculatePosition();

		glm::quat orientation = GetOrientation();
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		// 基于视口高度调整旋转速度，确保不同窗口大小下体验一致
		return 0.3f / (m_ViewportHeight / 1080.0f); // 以 1080p 为基准，窗口越小速度越慢
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		// 1. 处理鼠标捕获/释放（右键控制）
		if (Input_old::IsMouseButtonPressed(Mouse::ButtonRight))
		{
			if (!m_IsMouseCaptured)
			{
				// 按下右键：捕获鼠标（隐藏+锁定）
				m_IsMouseCaptured = true;
				m_LastMousePosition = { Input_old::GetMouseX(), Input_old::GetMouseY() };
				Input_old::SetCursorMode(Input_old::CursorMode::Locked); // 隐藏鼠标并锁定在窗口内
			}
		}
		else if (m_IsMouseCaptured)
		{
			// 松开右键：释放鼠标（显示+自由）
			m_IsMouseCaptured = false;
			Input_old::SetCursorMode(Input_old::CursorMode::Normal); // 恢复鼠标显示
		}

		// 2. 捕获状态下：处理鼠标旋转和 WASD 移动
		if (m_IsMouseCaptured)
		{
			// （1）鼠标旋转：计算鼠标位移，更新 Yaw/Pitch
			const glm::vec2 currentMousePos = { Input_old::GetMouseX(), Input_old::GetMouseY() };
			glm::vec2 mouseDelta = currentMousePos - m_LastMousePosition;
			m_LastMousePosition = currentMousePos;

			const float rotationSpeed = RotationSpeed() * ts; // 乘以时间步，确保帧率无关
			m_Yaw += mouseDelta.x * rotationSpeed;    // 水平位移 → 偏航角（左右旋转）
			m_Pitch += mouseDelta.y * rotationSpeed;  // 垂直位移 → 俯仰角（上下旋转，负号是因为鼠标上移=相机上仰）

			// 限制俯仰角：避免相机翻转（-89° ~ 89°，防止万向锁）
			m_Pitch = glm::clamp(m_Pitch, -glm::radians(89.0f), glm::radians(89.0f));

			// （2）WASD 移动：基于相机方向向量计算位移
			const float moveSpeed = 5.0f * ts; // 移动速度（可根据需求调整，乘以时间步确保帧率无关）
			glm::vec3 moveDir = { 0.0f, 0.0f, 0.0f };

			if (Input_old::IsKeyPressed(Key_old::W)) moveDir += GetForwardDirection();   // W → 前进（相机前向）
			if (Input_old::IsKeyPressed(Key_old::S)) moveDir -= GetForwardDirection();   // S → 后退
			if (Input_old::IsKeyPressed(Key_old::A)) moveDir -= GetRightDirection();     // A → 左移（相机右向的反方向）
			if (Input_old::IsKeyPressed(Key_old::D)) moveDir += GetRightDirection();     // D → 右移
			if (Input_old::IsKeyPressed(Key_old::LeftShift)) moveDir *= 2.0f;            // Shift → 加速（可选）
			if (Input_old::IsKeyPressed(Key_old::Space)) moveDir += GetUpDirection();    // Space → 上升（可选）
			if (Input_old::IsKeyPressed(Key_old::LeftControl)) moveDir -= GetUpDirection(); // Ctrl → 下降（可选）

			// 归一化移动方向：避免斜向移动速度过快
			if (glm::length(moveDir) > 0.0f)
				moveDir = glm::normalize(moveDir) * moveSpeed;

			// 更新焦点位置（因为 EditorCamera 是围绕焦点旋转，移动时需同步移动焦点）
			m_FocalPoint += moveDir;
		}

		// 3. 原有滚轮缩放逻辑（保留，与右键移动不冲突）
		// （注：滚轮缩放逻辑已在 OnMouseScroll 中处理，这里只需确保 UpdateView 被调用）

		// 4. 更新视图矩阵（无论哪种操作，最终都要刷新视图）
		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(HZ_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}


	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

}
