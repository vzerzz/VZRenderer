#pragma once
#include "geometry.h"

class Camera {
public:
	Camera(Vec3f eye, Vec3f target, Vec3f u, float aspect);

	Vec3f eye_pos;
	Vec3f target_pos;
	Vec3f up;
	Vec3f x;
	Vec3f y;
	Vec3f z;
	float aspect;

};

// Transform Matrix
Matrix ViewMatrix(Camera* camera);
Matrix PerspectiveMatrix(float fovY, float aspect, float n, float f);

//handle event
void updata_camera_pos(Camera& camera);
void handle_events(Camera& camera);
