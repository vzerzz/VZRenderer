#pragma once
#include <thread>
#include "geometry.h"
#include "tgaimage.h"

typedef struct cubemap 
{
	TGAImage *faces[6];
}cubemap_t;


class Texture {
private:
	void load_texture(std::string filename, const char* suffix, TGAImage* img);
	void load_cubemap(const char* filename);
public:
	TGAImage* diffusemap_;
	TGAImage* normalmap_;
	TGAImage* specularmap_;
	TGAImage* roughnessmap_;
	TGAImage* metalnessmap_;
	TGAImage* occlusionmap_;
	TGAImage* emissionmap_;
	Texture(const char* filename, bool isskybox);// 将文件中的.tga转为TGAImage
	~Texture();

	//skybox
	cubemap_t* environmentmap_;

	// texture sampling
	Vec3f diffuse_sampling(Vec2f uv);
	Vec3f normal_sampling(Vec2f uv);
	float specular_sampling(Vec2f uv);
	float roughness_sampling(Vec2f uv);
	float metalness_sampling(Vec2f uv);
	float occlusion_sampling(Vec2f uv);
	Vec3f emission_sampling(Vec2f uv);
	Vec3f cubemap_sampling(Vec3f world_pos);
};

Vec3f cubemap_sampling(cubemap_t* environmentmap_, Vec3f world_pos);
Vec3f texture_sampling(TGAImage* image, Vec2f uv);
