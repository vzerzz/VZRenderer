#include "camera.h"
#include "win.h"

Camera::Camera(Vec3f eye_pos, Vec3f target, Vec3f u, float aspect)
	: eye_pos(eye_pos), target_pos(target), up(u), aspect(aspect) {
	z = (eye_pos - target).normalize();
	x = cross(u, z).normalize();
	y = cross(z, x).normalize();
}

Matrix ViewMatrix(Camera* camera) {
	Matrix m = Matrix::identity(4);
	m[0][0] = camera->x[0];
	m[0][1] = camera->x[1];
	m[0][2] = camera->x[2];

	m[1][0] = camera->y[0];
	m[1][1] = camera->y[1];
	m[1][2] = camera->y[2];

	m[2][0] = camera->z[0];
	m[2][1] = camera->z[1];
	m[2][2] = camera->z[2];

	m[0][3] = -dot(camera->x, camera->eye_pos); //相当于原来要位移的，在新的坐标系下是位移多少，有个改变
	m[1][3] = -dot(camera->y, camera->eye_pos);
	m[2][3] = -dot(camera->z, camera->eye_pos);

	return m;
}

// right-handed system
Matrix PerspectiveMatrix(float fovY, float aspect, float n, float f) {
	Matrix m = Matrix::identity(4);
	float t = fabs(n) * tan(fovY / 180.f * PI / 2);
	float r = aspect * t;

	m[0][0] = n / r;
	m[1][1] = n / t;
	m[2][2] = (n + f) / (n - f);
	m[2][3] = 2 * n * f / (f - n);
	m[3][2] = 1;
	m[3][3] = 0;
	return m;
}

void updata_camera_pos(Camera& camera)
{
	Vec3f from_target = camera.eye_pos - camera.target_pos;			// vector point from target to camera's position
	float radius = from_target.norm();

	float phi = (float)atan2(from_target[0], from_target[2]); // azimuth angle(·½Î»½Ç), angle between from_target and z-axis£¬[-pi, pi]
	float theta = (float)acos(from_target[1] / radius);		  // zenith angle(Ìì¶¥½Ç), angle between from_target and y-axis, [0, pi]
	float x_delta = window->mouse_info.orbit_delta[0] / window->width;
	float y_delta = window->mouse_info.orbit_delta[1] / window->height;

	// for mouse wheel
	radius *= (float)pow(0.95, window->mouse_info.wheel_delta);

	float factor = 1.5 * PI;
	// for mouse left button
	phi += x_delta * factor;
	theta += y_delta * factor;
	if (theta > PI) theta = PI - EPSILON * 100;
	if (theta < 0)  theta = EPSILON * 100;

	camera.eye_pos[0] = camera.target_pos[0] + radius * sin(phi) * sin(theta);
	camera.eye_pos[1] = camera.target_pos[1] + radius * cos(theta);
	camera.eye_pos[2] = camera.target_pos[2] + radius * sin(theta) * cos(phi);

	// for mouse right button
	factor = radius * (float)tan(60.0 / 360 * PI) * 2.2;
	x_delta = window->mouse_info.fv_delta[0] / window->width;
	y_delta = window->mouse_info.fv_delta[1] / window->height;
	Vec3f left = x_delta * factor * camera.x;
	Vec3f up = y_delta * factor * camera.y;

	camera.eye_pos += (left - up);
	camera.target_pos += (left - up);
}

void handle_mouse_events(Camera& camera)
{
	if (window->buttons[0])
	{
		Vec2f cur_pos = get_mouse_pos();
		window->mouse_info.orbit_delta = window->mouse_info.orbit_pos - cur_pos;
		window->mouse_info.orbit_pos = cur_pos;
	}

	if (window->buttons[1])
	{
		Vec2f cur_pos = get_mouse_pos();
		window->mouse_info.fv_delta = window->mouse_info.fv_pos - cur_pos;
		window->mouse_info.fv_pos = cur_pos;
	}

	updata_camera_pos(camera);
}

void handle_key_events(Camera& camera)
{
	float distance = (camera.target_pos - camera.eye_pos).norm();

	if (window->keys['W'])
	{
		camera.eye_pos += (float)- 10.0 / window->width * camera.z * distance;
	}
	if (window->keys['S'])
	{
		camera.eye_pos += 0.05f * camera.z;
	}
	if (window->keys[VK_UP] || window->keys['Q'])
	{
		camera.eye_pos += 0.05f * camera.y;
		camera.target_pos += 0.05f * camera.y;
	}
	if (window->keys[VK_DOWN] || window->keys['E'])
	{
		camera.eye_pos += -0.05f * camera.y;
		camera.target_pos += -0.05f * camera.y;
	}
	if (window->keys[VK_LEFT] || window->keys['A'])
	{
		camera.eye_pos += -0.05f * camera.x;
		camera.target_pos += -0.05f * camera.x;
	}
	if (window->keys[VK_RIGHT] || window->keys['D'])
	{
		camera.eye_pos += 0.05f * camera.x;
		camera.target_pos += 0.05f * camera.x;
	}
	if (window->keys[VK_ESCAPE])
	{
		window->is_close = 1;
	}
}

void handle_events(Camera& camera)
{
	//calculate camera axis
	camera.z = (camera.eye_pos - camera.target_pos).normalize();
	camera.x = (cross(camera.up, camera.z)).normalize();
	camera.y = (cross(camera.z, camera.x)).normalize();

	//mouse and keyboard events
	handle_mouse_events(camera);
	handle_key_events(camera);
}