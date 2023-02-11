#include "shader.h"

static Vec3f TBN_normal(Vec3f& normal, Vec3f* world_coords, Vec2f* texture_coords, Vec2f& texture_coord, Vec3f& uv_normal) {
	float uAB = texture_coords[1][0] - texture_coords[0][0];
	float vAB = texture_coords[1][1] - texture_coords[0][1];
	float uAC = texture_coords[2][0] - texture_coords[0][0];
	float vAC = texture_coords[2][1] - texture_coords[0][1];
	Vec3f AB = world_coords[1] - world_coords[0];
	Vec3f AC = world_coords[2] - world_coords[0];
	Vec3f t = (AB * vAC - AC * vAB) * (1/(uAB * vAC - uAC * vAB));
	Vec3f b = (AB * (- uAC) + AC * uAB) * (1/(uAB * vAC - uAC * vAB));
	// schmidt orthogonalization
	Vec3f n = normal.normalize();
	t = (t - dot(t, n) * n).normalize();
	b = (b - dot(b, n) * n - dot(b, t) * t).normalize();
	Vec3f res = t * uv_normal[0] + b * uv_normal[1] + n * uv_normal[2];
	return res;

}

void PhongShader::vertex_shader(int nfaces, int nvertex) {
	Vec3f vert_ = payload.model->vert(nfaces, nvertex);

	payload.world_coords[nvertex] = vert_;
	payload.normals[nvertex] = payload.model->normal(nfaces, nvertex);
	payload.texture_coords[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.clip_coords[nvertex] = payload.mvpMatrix * to_Vec4(vert_);
}

Vec3f PhongShader::fragment_shader(float alpha, float beta, float gamma)
{
	// 插值 且矫正
	float Z = 1.f / (alpha / payload.clip_coords[0].w + beta / payload.clip_coords[1].w + gamma / payload.clip_coords[2].w);
	Vec3f world_coord = (payload.world_coords[0] * (alpha / payload.clip_coords[0].w) + payload.world_coords[1] * (beta / payload.clip_coords[1].w) + payload.world_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;
	Vec3f normal = (payload.normals[0] * (alpha / payload.clip_coords[0].w) + payload.normals[1] * (beta / payload.clip_coords[1].w) + payload.normals[2] * (gamma / payload.clip_coords[2].w)) * Z;
	Vec2f texture_coord = (payload.texture_coords[0] * (alpha / payload.clip_coords[0].w) + payload.texture_coords[1] * (beta / payload.clip_coords[1].w) + payload.texture_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;

	// 切线空间法线贴图
	if (payload.model->texture->normalmap_) {
		Vec3f uv_normal = payload.model->texture->normal_sampling(texture_coord);
		normal = TBN_normal(normal, payload.world_coords, payload.texture_coords, texture_coord, uv_normal);
	}

	//Blinn-Phong 
	light_t light{ Vec3f(0.05f, 0.05f, 0.05f), Vec3f(0.8f, 0.8f, 0.8f), Vec3f(0.5f, 0.5f, 0.5f), Vec3f(1.f, 1.f, 1.f) };
	Vec3f ka(0.35f, 0.35f, 0.35f);
	Vec3f kd = payload.model->texture->diffuse_sampling(texture_coord);
	Vec3f ks(0.8f, 0.8f, 0.8f);
	float shininess = 64.f;

	Vec3f light_dir = light.direction.normalize();
	Vec3f view_dir = (payload.camera->eye_pos - world_coord).normalize();
	Vec3f halfway_dir = (light_dir + view_dir).normalize();
	Vec3f normal_dir = normal.normalize();

	Vec3f ambient = ka * light.ambient;
	Vec3f diffuse = kd * light.diffuse * std::max(0.f, dot(halfway_dir, normal_dir));
	Vec3f specular = ks * light.specular * std::pow(std::max(0.f, dot(halfway_dir, normal_dir)), shininess);

	return (ambient + diffuse + specular) * 255.f;
}
