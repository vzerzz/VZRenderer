#include "scene.h"

TGAImage *texture_from_file(const char *file_name)
{
	TGAImage *image = new TGAImage();
	image->read_tga_file(file_name);
	image->flip_vertically();
	return image;
}

cubemap_t *cubemap_from_files(const char *positive_x, const char *negative_x,
	const char *positive_y, const char *negative_y,
	const char *positive_z, const char *negative_z)
{
	cubemap_t *cubemap = new cubemap_t();
	cubemap->faces[0] = texture_from_file(positive_x);
	cubemap->faces[1] = texture_from_file(negative_x);
	cubemap->faces[2] = texture_from_file(positive_y);
	cubemap->faces[3] = texture_from_file(negative_y);
	cubemap->faces[4] = texture_from_file(positive_z);
	cubemap->faces[5] = texture_from_file(negative_z);
	return cubemap;
}

void load_ibl_map(payload_t &p, const char* env_path)
{
	int i, j;
	iblmap_t *iblmap = new iblmap_t();
	const char *faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];

	iblmap->mip_levels = 10;

	/* diffuse environment map */
	for (j = 0; j < 6; j++) {
		sprintf_s(paths[j], "%s/i_%s.tga", env_path, faces[j]);
	}
	iblmap->irradiance_map = cubemap_from_files(paths[0], paths[1], paths[2],
		paths[3], paths[4], paths[5]);

	/* specular environment maps */
	for (i = 0; i < iblmap->mip_levels; i++) {
		for (j = 0; j < 6; j++) {
			sprintf_s(paths[j], "%s/m%d_%s.tga", env_path, i, faces[j]);
		}
		iblmap->prefilter_maps[i] = cubemap_from_files(paths[0], paths[1],
			paths[2], paths[3], paths[4], paths[5]);
	}

	/* brdf lookup texture */
	char path[50] = "\0";
	strcat(path, env_path);
	const char* path_lut = "\\BRDF_LUT.tga";
	strcat(path, path_lut);
	iblmap->brdf_lut = texture_from_file(path);

	p.iblmap = iblmap;

}

void Scene::helmet_lake_scene()
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\helmet\\helmet.obj",
		"..\\..\\obj\\lake\\lake.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* model_shader = new PBRShader();
	IShader* skybox_shader = new SkyboxShader();
	model_shader->payload.camera = camera;
	skybox_shader->payload.camera = camera;

	load_ibl_map(model_shader->payload, "..\\..\\obj\\lake\\preprocessing");

	*_model_shader = model_shader;
	*_skybox_shader = skybox_shader;

}

void Scene::helmet_indoor_scene()
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\helmet\\helmet.obj",
		"..\\..\\obj\\indoor\\indoor.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* model_shader = new PBRShader();
	IShader* skybox_shader = new SkyboxShader();
	model_shader->payload.camera = camera;
	skybox_shader->payload.camera = camera;

	load_ibl_map(model_shader->payload, "..\\..\\obj\\indoor\\preprocessing");

	*_model_shader = model_shader;
	*_skybox_shader = skybox_shader;
}

void Scene::cerberus_lake_scene()
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\cerberus\\cerberus.obj",
		"..\\..\\obj\\lake\\lake.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* model_shader = new PBRShader();
	IShader* skybox_shader = new SkyboxShader();
	model_shader->payload.camera = camera;
	skybox_shader->payload.camera = camera;

	load_ibl_map(model_shader->payload, "..\\..\\obj\\lake\\preprocessing");

	*_model_shader = model_shader;
	*_skybox_shader = skybox_shader;
}

void Scene::cerberus_indoor_scene()
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\cerberus\\cerberus.obj",
		"..\\..\\obj\\indoor\\indoor.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* model_shader = new PBRShader();
	IShader* skybox_shader = new SkyboxShader();
	model_shader->payload.camera = camera;
	skybox_shader->payload.camera = camera;

	load_ibl_map(model_shader->payload, "..\\..\\obj\\indoor\\preprocessing");

	*_model_shader = model_shader;
	*_skybox_shader = skybox_shader;
}

Scene::~Scene()
{
	for (int i = 0; i < m; i++) if (model[i] != nullptr) { delete model[i]; model[i] = nullptr; }
	IShader* model_shader = *_model_shader;
	IShader* skybox_shader = *_skybox_shader;
	delete[] model_shader->payload.iblmap->irradiance_map->faces;
	for (int i = 0; i <  model_shader->payload.iblmap->mip_levels; i++) 
		delete[] model_shader->payload.iblmap->prefilter_maps[i]->faces;
	delete model_shader->payload.iblmap->brdf_lut;
	delete model_shader->payload.iblmap;
	if (model_shader != nullptr) { delete model_shader; model_shader = nullptr; }
	if (skybox_shader != nullptr) { delete skybox_shader; skybox_shader = nullptr; }

}

