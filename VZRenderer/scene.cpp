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

void helmet_lake_scene(Model** model, int& m, IShader** model_shader, IShader** skybox_shader, Camera* camera)
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\helmet\\helmet.obj",
		"..\\..\\obj\\lake\\lake.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* modelshader = new PBRShader();
	IShader* skyboxshader = new SkyboxShader();
	modelshader->payload.camera = camera;
	skyboxshader->payload.camera = camera;

	load_ibl_map(modelshader->payload, "..\\..\\obj\\lake\\preprocessing");

	*model_shader = modelshader;
	*skybox_shader = skyboxshader;
}

void helmet_indoor_scene(Model** model, int& m, IShader** model_shader, IShader** skybox_shader, Camera* camera)
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\helmet\\helmet.obj",
		"..\\..\\obj\\indoor\\indoor.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* modelshader = new PBRShader();
	IShader* skyboxshader = new SkyboxShader();
	modelshader->payload.camera = camera;
	skyboxshader->payload.camera = camera;

	load_ibl_map(modelshader->payload, "..\\..\\obj\\indoor\\preprocessing");

	*model_shader = modelshader;
	*skybox_shader = skyboxshader;

}

void cerberus_lake_scene(Model** model, int& m, IShader** model_shader, IShader** skybox_shader, Camera* camera)
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\cerberus\\cerberus.obj",
		"..\\..\\obj\\lake\\lake.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* modelshader = new PBRShader();
	IShader* skyboxshader = new SkyboxShader();
	modelshader->payload.camera = camera;
	skyboxshader->payload.camera = camera;

	load_ibl_map(modelshader->payload, "..\\..\\obj\\lake\\preprocessing");

	*model_shader = modelshader;
	*skybox_shader = skyboxshader;

}

void cerberus_indoor_scene(Model** model, int& m, IShader** model_shader, IShader** skybox_shader, Camera* camera)
{
	m = 2;
	const char* model_name[] = {
		"..\\..\\obj\\cerberus\\cerberus.obj",
		"..\\..\\obj\\indoor\\indoor.obj"
	};
	model[0] = new Model(model_name[0], false);
	model[1] = new Model(model_name[1], true);

	IShader* modelshader = new PBRShader();
	IShader* skyboxshader = new SkyboxShader();
	modelshader->payload.camera = camera;
	skyboxshader->payload.camera = camera;

	load_ibl_map(modelshader->payload, "..\\..\\obj\\indoor\\preprocessing");

	*model_shader = modelshader;
	*skybox_shader = skyboxshader;

}
