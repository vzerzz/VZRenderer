#pragma once
#include "shader.h"

class Scene {

private:
	Model** model;
	int& m;
	IShader** _model_shader;
	IShader** _skybox_shader;
	Camera* camera;
	int index;
	void helmet_lake_scene();
	void helmet_indoor_scene();
	void cerberus_lake_scene();
	void cerberus_indoor_scene();

public:
	Scene(Model** model_, int& m_, IShader** model_shader_, IShader** skybox_shader_, Camera* camera_, int index_)
		:model(model_), m(m_), _model_shader(model_shader_), _skybox_shader(skybox_shader_), camera(camera_), index(index_) {
		switch (index) {
		case 0:
			helmet_lake_scene();
			break;
		case 1:
			helmet_indoor_scene();
			break;
		case 2:
			cerberus_lake_scene();
			break;
		case 3:
			cerberus_indoor_scene();
			break;
		default:
			break;
		}
	}

	~Scene();

};


