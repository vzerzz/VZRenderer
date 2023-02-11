#include "texture.h"
#include <io.h>

void Texture::load_texture(std::string filename, const char* suffix, TGAImage* img) {
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != std::string::npos) {
		texfile = texfile.substr(0, dot) + std::string(suffix);
		img->read_tga_file(texfile.c_str());
		img->flip_vertically();
	}
}

void Texture::load_cubemap(const char* filename)
{
	environmentmap_->faces[0] = new TGAImage();
	load_texture(filename, "_right.tga", environmentmap_->faces[0]);
	environmentmap_->faces[1] = new TGAImage();
	load_texture(filename, "_left.tga", environmentmap_->faces[1]);
	environmentmap_->faces[2] = new TGAImage();
	load_texture(filename, "_top.tga", environmentmap_->faces[2]);
	environmentmap_->faces[3] = new TGAImage();
	load_texture(filename, "_bottom.tga", environmentmap_->faces[3]);
	environmentmap_->faces[4] = new TGAImage();
	load_texture(filename, "_back.tga", environmentmap_->faces[4]);
	environmentmap_->faces[5] = new TGAImage();
	load_texture(filename, "_front.tga", environmentmap_->faces[5]);
}

Texture::Texture(const char* filename, bool isskybox) {
	diffusemap_ = NULL;
	normalmap_ = NULL;
	specularmap_ = NULL;
	roughnessmap_ = NULL;
	metalnessmap_ = NULL;
	occlusionmap_ = NULL;
	emissionmap_ = NULL;
	environmentmap_ = nullptr;

	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");

	texfile = texfile.substr(0, dot) + std::string("_diffuse.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		diffusemap_ = new TGAImage();
		load_texture(filename, "_diffuse.tga", diffusemap_);
	}

	texfile = texfile.substr(0, dot) + std::string("_normal.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		normalmap_ = new TGAImage();
		load_texture(filename, "_normal.tga", normalmap_);
	}

	texfile = texfile.substr(0, dot) + std::string("_specular.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		specularmap_ = new TGAImage();
		load_texture(filename, "_specular.tga", specularmap_);
	}

	texfile = texfile.substr(0, dot) + std::string("_roughness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		roughnessmap_ = new TGAImage();
		load_texture(filename, "_roughness.tga", roughnessmap_);
	}

	texfile = texfile.substr(0, dot) + std::string("_metalness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		metalnessmap_ = new TGAImage();
		load_texture(filename, "_metalness.tga", metalnessmap_);
	}

	texfile = texfile.substr(0, dot) + std::string("_emission.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		emissionmap_ = new TGAImage();
		load_texture(filename, "_emission.tga", emissionmap_);
	}

	texfile = texfile.substr(0, dot) + std::string("_occlusion.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		occlusionmap_ = new TGAImage();
		load_texture(filename, "_occlusion.tga", occlusionmap_);
	}

	if (isskybox) {
		environmentmap_ = new cubemap_t();
		load_cubemap(filename);
	}

}

Texture::~Texture() {
	if (diffusemap_) delete diffusemap_; diffusemap_ = NULL;
	if (normalmap_) delete normalmap_; normalmap_ = NULL;
	if (specularmap_) delete specularmap_; specularmap_ = NULL;
	if (roughnessmap_) delete roughnessmap_; roughnessmap_ = NULL;
	if (metalnessmap_) delete metalnessmap_; metalnessmap_ = NULL;
	if (occlusionmap_) delete occlusionmap_; occlusionmap_ = NULL;
	if (emissionmap_) delete emissionmap_; emissionmap_ = NULL;
	if (environmentmap_) {
		for (int i = 0; i < 6; i++) delete environmentmap_->faces[i];
		delete environmentmap_; environmentmap_ = nullptr;
	}
}

// texture sampling
Vec3f Texture::diffuse_sampling(Vec2f uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * diffusemap_->get_width();
	int uv1 = uv[1] * diffusemap_->get_height();
	TGAColor color = diffusemap_->get(uv0, uv1);// TGAColor中以bgra存储
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)color[i] / 255.f;// 输出0~1范围内的RGB
	return res;
}

Vec3f Texture::normal_sampling(Vec2f uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * normalmap_->get_width();
	int uv1 = uv[1] * normalmap_->get_height();
	TGAColor c = normalmap_->get(uv0, uv1);
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f; //because the normap_map coordinate is -1 ~ +1
	return res;
}

float Texture::roughness_sampling(Vec2f uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * roughnessmap_->get_width();
	int uv1 = uv[1] * roughnessmap_->get_height();
	return roughnessmap_->get(uv0, uv1)[0] / 255.f;
}

float Texture::metalness_sampling(Vec2f uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * metalnessmap_->get_width();
	int uv1 = uv[1] * metalnessmap_->get_height();
	return metalnessmap_->get(uv0, uv1)[0] / 255.f;
}

float Texture::specular_sampling(Vec2f uv)
{
	int uv0 = uv[0] * specularmap_->get_width();
	int uv1 = uv[1] * specularmap_->get_height();
	return specularmap_->get(uv0, uv1)[0] / 1.f;
}

float Texture::occlusion_sampling(Vec2f uv)
{
	if (!occlusionmap_)
		return 1;
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * occlusionmap_->get_width();
	int uv1 = uv[1] * occlusionmap_->get_height();
	return occlusionmap_->get(uv0, uv1)[0] / 255.f;
}

Vec3f Texture::emission_sampling(Vec2f uv)
{
	if (!emissionmap_)
		return Vec3f(0.0f, 0.0f, 0.0f);
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * emissionmap_->get_width();
	int uv1 = uv[1] * emissionmap_->get_height();
	TGAColor c = emissionmap_->get(uv0, uv1);
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f;
	return res;
}


int cal_cubemap_uv(Vec3f world_pos, Vec2f& uv)
{
	int face_index = -1;
	float ma = 0, sc = 0, tc = 0;
	float abs_x = fabs(world_pos[0]), abs_y = fabs(world_pos[1]), abs_z = fabs(world_pos[2]);
	if (abs_x > abs_y && abs_x > abs_z) {		/* major axis -> x */
		ma = abs_x;
		if (world_pos.x > 0) {					/* positive x */
			face_index = 0;
			sc = +world_pos.z;
			tc = +world_pos.y;
		}
		else {									/* negative x */
			face_index = 1;
			sc = -world_pos.z;
			tc = +world_pos.y;
		}
	}
	else if (abs_y > abs_z) {					/* major axis -> y */
		ma = abs_y;
		if (world_pos.y > 0) {			/* positive y */
			face_index = 2;
			sc = +world_pos.x;
			tc = +world_pos.z;
		}
		else {								/* negative y */
			face_index = 3;
			sc = +world_pos.x;
			tc = -world_pos.z;
		}
	}
	else {								/* major axis -> z */
		ma = abs_z;
		if (world_pos.z > 0) {				/* positive z */
			face_index = 4;
			sc = -world_pos.x;
			tc = +world_pos.y;
		}
		else {									/* negative z */
			face_index = 5;
			sc = +world_pos.x;
			tc = +world_pos.y;
		}
	}
	uv[0] = (sc / ma + 1.0f) / 2.0f;
	uv[1] = (tc / ma + 1.0f) / 2.0f;
	return face_index;
}

Vec3f Texture::cubemap_sampling(Vec3f world_pos)
{
	Vec2f uv;
	int face_index = cal_cubemap_uv(world_pos, uv);
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * environmentmap_->faces[face_index]->get_width();
	int uv1 = uv[1] * environmentmap_->faces[face_index]->get_height();
	TGAColor color = environmentmap_->faces[face_index]->get(uv0, uv1);
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)color[i];
	return res;
}

Vec3f cubemap_sampling(cubemap_t* environmentmap_, Vec3f world_pos)
{
	Vec2f uv;
	int face_index = cal_cubemap_uv(world_pos, uv);
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * environmentmap_->faces[face_index]->get_width();
	int uv1 = uv[1] * environmentmap_->faces[face_index]->get_height();
	TGAColor color = environmentmap_->faces[face_index]->get(uv0, uv1);
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)color[i]/255.f;
	return res;
}

Vec3f texture_sampling(TGAImage* image, Vec2f uv)
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * image->get_width();
	int uv1 = uv[1] * image->get_height();
	TGAColor color = image->get(uv0, uv1);// TGAColor中以bgra存储
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)color[i] / 255.f;// 输出0~1范围内的RGB
	return res;
}
