#include <vector>
#include <cmath>
#include <iostream>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "shader.h"
#include "camera.h"
#include "rasterizer.h"
#include "win.h"
#include "ibl.h"
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )//不显示控制台

/*
int main(){
	foreach_irradiance_map();
	foreach_prefilter_miplevel();
	BRDF_LUT();
	return 0;
}
*/

void load_ibl_map(payload_t& p, const char* env_path);

int main(int argc, char** argv) {
	Camera* camera = new Camera(Vec3f(0, 1, 5), Vec3f(0, 1, 0), Vec3f(0, 1, 0), (float)WINDOW_WIDTH / WINDOW_HEIGHT);
	//TGAImage image(WINDOW_WIDTH, WINDOW_HEIGHT, TGAImage::RGB);
	unsigned char* framebuffer = new unsigned char[WINDOW_HEIGHT * WINDOW_WIDTH * 4];
	float* zbuffer = new float[WINDOW_WIDTH * WINDOW_HEIGHT];
	for (int i = WINDOW_WIDTH * WINDOW_HEIGHT; i--; zbuffer[i] = -(std::numeric_limits<float>::max)());

	Matrix modelMatrix = Matrix::identity(4);
	Matrix viewMatrix = ViewMatrix(camera);
	Matrix perspectiveMatrix = PerspectiveMatrix(60, (float)WINDOW_WIDTH / WINDOW_HEIGHT, -0.3, -30000);
	Matrix mvpMatrix = perspectiveMatrix * viewMatrix * modelMatrix;

	// load model
	Model* model[10];
	int model_num = 2;
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

	// Render loop
	window_init(WINDOW_WIDTH, WINDOW_HEIGHT, "VZRenderer");
	int num_frames = 0;
	float print_time = platform_get_time();
	while (!window->is_close) {
		float cur_time = platform_get_time();

		// clear buffer
		for (int i = WINDOW_WIDTH * WINDOW_HEIGHT; i--; zbuffer[i] = (std::numeric_limits<float>::max)());// 不加()此处的max会和windows.h中max宏冲突
		for (int i = 0; i < WINDOW_HEIGHT; i++)
			for (int j = 0; j < WINDOW_WIDTH; j++) {
				framebuffer[(i * WINDOW_WIDTH + j) * 4] = 56;
				framebuffer[(i * WINDOW_WIDTH + j) * 4 + 1] = 56;
				framebuffer[(i * WINDOW_WIDTH + j) * 4 + 2] = 80;
			}
		// handle events
		handle_events(*camera);

		// update viewMatrix
		viewMatrix = ViewMatrix(camera);
		mvpMatrix = perspectiveMatrix * viewMatrix * modelMatrix;

		model_shader->payload.mvpMatrix = mvpMatrix;

		viewMatrix[0][3] = 0;
		viewMatrix[1][3] = 0;
		viewMatrix[2][3] = 0;
		skybox_shader->payload.mvpMatrix = perspectiveMatrix * viewMatrix;
		//draw
		for (int j = 0; j < model_num; j++) {
			if (!model[j]->skybox) {
				model_shader->payload.model = model[j];
				for (int i = 0; i < model[j]->nfaces(); i++)
					draw(framebuffer, zbuffer, *model_shader, i);
			}
			else {
				skybox_shader->payload.model = model[j];
				for (int i = 0; i < model[j]->nfaces(); i++)
					draw(framebuffer, zbuffer, *skybox_shader, i);
			}
		}


		// calculate and display FPS
		int temp_num, temp_avg;

		num_frames += 1;
		if (cur_time - print_time >= 1) {
			int sum_millis = (int)((cur_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;
			temp_num = num_frames; temp_avg = avg_millis;
			num_frames = 0;
			print_time = cur_time;
		}

		// reset mouse information
		window->mouse_info.wheel_delta = 0;
		window->mouse_info.orbit_delta = Vec2f(0, 0);
		window->mouse_info.fv_delta = Vec2f(0, 0);

		// send framebuffer to window 
		window_draw(framebuffer);
		show_para(temp_num, temp_avg);
		msg_dispatch();

	}

	//image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	//image.write_tga_file("output.tga");

	delete[]model;
	delete camera;
	delete model_shader;
	delete skybox_shader;
	delete[]zbuffer;
	delete[]framebuffer;

	system("pause");
	return 0;
}

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
	iblmap->brdf_lut = texture_from_file("..\\..\\obj\\lake\\preprocessing\\BRDF_LUT.tga");

	p.iblmap = iblmap;

}
