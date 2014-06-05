#pragma once

#include "glm/glm.hpp"

class Camera
{
public:
	Camera(const glm::mat4& p_projection);

	void SetPosition(const glm::vec3& p_position);
	void SetFacing(const glm::vec3& p_facing);
	void LookAt(const glm::vec3& p_position);
	void Yaw(float p_angle);
	void Pitch(float p_angle);

	void Commit();

	const glm::vec3& GetPosition() const;
	const glm::vec3& GetFacing() const;
	glm::vec3 GetRight() const;
	const glm::mat4& GetView() const;
	const glm::mat4& GetProjection() const;
	const glm::mat4& GetViewProjection() const;
	const glm::mat4& GetInverseViewProjection() const;

	static glm::mat4 CreatePerspectiveProjection(float p_near, float p_far, float p_fovY, float p_aspect);
private:
	glm::vec3 m_position;
	glm::vec3 m_facing;

	glm::mat4 m_view;
	glm::mat4 m_projection;
	glm::mat4 m_viewProjection;
	glm::mat4 m_inverseViewProjection;
};