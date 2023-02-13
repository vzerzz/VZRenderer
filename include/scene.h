#pragma once
#include "shader.h"

typedef struct 
{
	const char *scene_name;
	void (*build_scene)(Model **model, int& m, IShader **model_shader, IShader **skybox_shader, Camera *camera);
} scene_t;

void helmet_lake_scene(Model **model, int& m, IShader **model_shader, IShader **skybox_shader, Camera *camera);
void helmet_indoor_scene(Model **model, int& m, IShader **model_shader, IShader **skybox_shader, Camera *camera);
void cerberus_lake_scene(Model **model, int& m, IShader **model_shader, IShader **skybox_shader, Camera *camera);
void cerberus_indoor_scene(Model **model, int& m, IShader **model_shader, IShader **skybox_shader, Camera *camera);
