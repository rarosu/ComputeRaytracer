#include "Camera.h"
#include "glm/gtx/transform.hpp"

const glm::vec3 Camera::UP(0.0f, -1.0f, 0.0f);

Camera::Camera( const glm::mat4& p_projection )
	: m_projection(p_projection)
	, m_facing(0, 0, 1)
{
	Commit();
}


void Camera::SetPosition( const glm::vec3& p_position )
{
	m_position = p_position;
}

void Camera::SetFacing( const glm::vec3& p_facing )
{
	m_facing = glm::normalize(p_facing);
}

void Camera::LookAt( const glm::vec3& p_position )
{
	m_facing = glm::normalize(p_position - m_position);
}


void Camera::Yaw( float p_angle )
{
	glm::mat4 m;
	m = glm::rotate(m, p_angle, UP);
	m_facing = glm::normalize(glm::vec3(m * glm::vec4(m_facing, 0.0f)));
}

void Camera::Pitch( float p_angle )
{
	glm::vec3 right = glm::cross(m_facing, UP);

	glm::mat4 m;
	m = glm::rotate(m, p_angle, right);
	m_facing = glm::normalize(glm::vec3(m * glm::vec4(m_facing, 0.0f)));
}



void Camera::Commit()
{
	// TODO: Maybe transpose
	m_view = glm::transpose(glm::lookAt(m_position, m_position + m_facing, UP));

	m_viewProjection = m_view * m_projection;
	m_inverseViewProjection = glm::inverse(m_viewProjection);
}

const glm::vec3& Camera::GetPosition() const
{
	return m_position;
}

const glm::vec3& Camera::GetFacing() const
{
	return m_facing;
}

glm::vec3 Camera::GetRight() const
{
	return glm::cross(m_facing, UP);
}

const glm::mat4& Camera::GetView() const
{
	return m_view;
}

const glm::mat4& Camera::GetProjection() const
{
	return m_projection;
}

const glm::mat4& Camera::GetViewProjection() const
{
	return m_viewProjection;
}

const glm::mat4& Camera::GetInverseViewProjection() const
{
	return m_inverseViewProjection;
}

glm::mat4 Camera::CreatePerspectiveProjection( float p_near, float p_far, float p_fovY, float p_aspect )
{
	// TODO: Maybe transpose
	return glm::transpose(glm::perspectiveFov(p_fovY, p_aspect, 1.0f, p_near, p_far));
}
