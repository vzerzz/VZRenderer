#pragma once
#include "geometry.h"
#include "model.h"
#include "camera.h"
#include "texture.h"

typedef struct light {
	Vec3f position;
	Vec3f ambient;
	Vec3f diffuse;
	Vec3f specular;
	Vec3f direction;
	Vec3f radiance;
}light_t;

typedef struct iblmap {
	int mip_levels;
	cubemap_t *irradiance_map;
	cubemap_t *prefilter_maps[15];
	TGAImage *brdf_lut;
}iblmap_t;

typedef struct {
	//在main中赋值
	Model* model;
	Camera* camera;
	Matrix modelMatrix;
	Matrix mvpMatrix;
	//vertexshader中赋值
	//vertex attribute
	Vec3f world_coords[3];
	Vec4f clip_coords[3];
	Vec2f texture_coords[3];
	Vec3f normals[3];

	// ibl
	iblmap_t* iblmap;
}payload_t;



class IShader {
public:
	payload_t payload;
	virtual void vertex_shader(int nfaces, int nvertex) = 0;
	virtual Vec3f fragment_shader(float alpha, float beta, float gamma) = 0;
};

class PhongShader :public IShader {
public:
	void vertex_shader(int nfaces, int nvertex);
	Vec3f fragment_shader(float alpha, float beta, float gamma);
};

class SkyboxShader :public IShader {
public:
	void vertex_shader(int nfaces, int nvertex);
	Vec3f fragment_shader(float alpha, float beta, float gamma);
};

class PBRShader :public IShader {
public:
	void vertex_shader(int nfaces, int nvertex);
	Vec3f fragment_shader_direct(float alpha, float beta, float gamma);
	Vec3f fragment_shader(float alpha, float beta, float gamma);

};
