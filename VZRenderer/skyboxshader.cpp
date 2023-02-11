#include "shader.h"

void SkyboxShader::vertex_shader(int nfaces, int nvertex) {
	Vec3f vert_ = payload.model->vert(nfaces, nvertex);

	payload.world_coords[nvertex] = vert_;
	payload.normals[nvertex] = payload.model->normal(nfaces, nvertex);
	payload.texture_coords[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.clip_coords[nvertex] = payload.mvpMatrix * to_Vec4(vert_);
}

Vec3f SkyboxShader::fragment_shader(float alpha, float beta, float gamma)
{
	float Z = 1.f / (alpha / payload.clip_coords[0].w + beta / payload.clip_coords[1].w + gamma / payload.clip_coords[2].w);
	Vec3f world_coord = (payload.world_coords[0] * (alpha / payload.clip_coords[0].w) + payload.world_coords[1] * (beta / payload.clip_coords[1].w) + payload.world_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;
	
	return payload.model->texture->cubemap_sampling(world_coord);
}

