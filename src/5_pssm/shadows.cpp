#include "shadows.h"
#include <camera.h>

Shadows::Shadows()
{

}

Shadows::~Shadows()
{

}

void Shadows::initialize(Camera* camera)
{

}

void Shadows::update(Camera* camera)
{

}

void Shadows::update_splits(Camera* camera)
{
	float nd = camera->m_near;
	float fd = camera->m_far;

	float lambda = m_settings.lambda;
	float ratio = fd / nd;
	m_splits[0].near = nd;

	for (int i = 1; i < m_settings.split_count; i++)
	{
		float si = i / (float)m_settings.split_count;

		// Practical Split Scheme: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		float t_near = lambda * (nd * powf(ratio, si)) + (1 - lambda) * (nd + (fd - nd) * si);
		float t_far = t_near * 1.005f;
		m_splits[i].near = t_near;
		m_splits[i - 1].far = t_far;
	}

	m_splits[m_settings.split_count - 1].far = fd;
}

void Shadows::update_frustum_corners(Camera* camera)
{
	glm::vec3 center = camera->m_position;
	glm::vec3 view_dir = camera->m_forward;

	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::cross(view_dir, up);

	for (int i = 0; i < m_settings.split_count; i++)
	{
		FrustumSplit& t_frustum = m_splits[i];

		glm::vec3 fc = center + view_dir * t_frustum.far;
		glm::vec3 nc = center + view_dir * t_frustum.near;

		right = glm::normalize(right);
		up = glm::normalize(glm::cross(right, view_dir));

		// these heights and widths are half the heights and widths of
		// the near and far plane rectangles
		float near_height = tan(t_frustum.fov / 2.0f) * t_frustum.near;
		float near_width = near_height * t_frustum.ratio;
		float far_height = tan(t_frustum.fov / 2.0f) * t_frustum.far;
		float far_width = far_height * t_frustum.ratio;

		t_frustum.corners[0] = nc - up * near_height - right * near_width; // near-bottom-left
		t_frustum.corners[1] = nc + up * near_height - right * near_width; // near-top-left
		t_frustum.corners[2] = nc + up * near_height + right * near_width; // near-top-right
		t_frustum.corners[3] = nc - up * near_height + right * near_width; // near-bottom-right

		t_frustum.corners[4] = fc - up * far_height - right * far_width; // far-bottom-left
		t_frustum.corners[5] = fc + up * far_height - right * far_width; // far-top-left
		t_frustum.corners[6] = fc + up * far_height + right * far_width; // far-top-right
		t_frustum.corners[7] = fc - up * far_height + right * far_width; // far-bottom-right
	}
}