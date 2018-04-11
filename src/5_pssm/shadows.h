#pragma once

#include <glm.hpp>

#define MAX_FRUSTUM_SPLITS 8

class Camera;

struct FrustumSplit
{
	float near;
	float far;
	float ratio;
	float fov;
	glm::vec3 corners[8];
};

struct ShadowSettings
{
	float lambda;
	int split_count;
};

class Shadows
{
private:
	ShadowSettings m_settings;
	FrustumSplit m_splits[MAX_FRUSTUM_SPLITS];
	glm::mat4 m_view;
	glm::mat4 m_crop_matrices[MAX_FRUSTUM_SPLITS];
	glm::mat4 m_proj_matrices[MAX_FRUSTUM_SPLITS];

public:
	Shadows();
	~Shadows();
	void initialize(Camera* camera);
	void update(Camera* camera);
	void update_splits(Camera* camera);
	void update_frustum_corners(Camera* camera);
};