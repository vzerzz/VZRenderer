#include <vector>
#include <cmath>
#include <iostream>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "shader.h"
#include "camera.h"
#include "pipeline.h"
#include "win.h"
#include "ibl.h"
#include "scene.h"
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )//不显示控制台

#define MAX_MODEL_NUM 10

//int main(){// 预计算
//	foreach_irradiance_map();
//	foreach_prefilter_miplevel();
//	BRDF_LUT();
//	return 0;
//}


int main(int argc, char** argv) {
	window_init(WINDOW_WIDTH, WINDOW_HEIGHT, "VZRenderer");
	Camera* camera = new Camera(Vec3f(0, 1, 5), Vec3f(0, 1, 0), Vec3f(0, 1, 0), (float)WINDOW_WIDTH / WINDOW_HEIGHT);
	unsigned char* framebuffer = new unsigned char[WINDOW_HEIGHT * WINDOW_WIDTH * 4];
	float* zbuffer = new float[WINDOW_WIDTH * WINDOW_HEIGHT];

	IShader* model_shader = nullptr;
	IShader* skybox_shader = nullptr;
	Model* model[MAX_MODEL_NUM];
	int model_num = 0;

	while (!window->is_close) {
		// load scene
		*camera = Camera(Vec3f(0, 1, 5), Vec3f(0, 1, 0), Vec3f(0, 1, 0), (float)WINDOW_WIDTH / WINDOW_HEIGHT);
		if (window->scene_index != -1) {
			Scene s(model, model_num, &model_shader, &skybox_shader, camera, window->scene_index);
			// Render loop
			int num_frames = 0;
			float print_time = platform_get_time();
			int show_num = 0, show_avg = 0;

			while (window->is_start) {
				if (window->is_close) break;
				window->scene_index = -1;
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

				// update Matrix
				Matrix viewMatrix = ViewMatrix(camera);
				Matrix perspectiveMatrix = PerspectiveMatrix(60, (float)WINDOW_WIDTH / WINDOW_HEIGHT, -0.3, -30000);

				model_shader->payload.mvpMatrix = perspectiveMatrix * viewMatrix * model_shader->payload.modelMatrix;

				viewMatrix[0][3] = 0;
				viewMatrix[1][3] = 0;
				viewMatrix[2][3] = 0;
				skybox_shader->payload.mvpMatrix = perspectiveMatrix * viewMatrix;
				
				//draw
				for (int j = 0; j < model_num; j++) {
					if (!model[j]->skybox) {
						model_shader->payload.model = model[j];
						for (int i = 0; i < model[j]->nfaces(); i++)
							DrawCall(framebuffer, zbuffer, *model_shader, i);
					}
					else {
						skybox_shader->payload.model = model[j];
						for (int i = 0; i < model[j]->nfaces(); i++)
							DrawCall(framebuffer, zbuffer, *skybox_shader, i);
					}
				}

				// calculate and display FPS
				num_frames += 1;
				if (cur_time - print_time >= 1) {
					int sum_millis = (int)((cur_time - print_time) * 1000);
					int avg_millis = sum_millis / num_frames;
					show_num = num_frames; show_avg = avg_millis;
					num_frames = 0;
					print_time = cur_time;
				}

				// reset mouse information
				window->mouse_info.wheel_delta = 0;
				window->mouse_info.orbit_delta = Vec2f(0, 0);
				window->mouse_info.fv_delta = Vec2f(0, 0);

				// send framebuffer to window 
				window_draw(framebuffer, show_num, show_avg);
				msg_dispatch();
			}
		}
		msg_dispatch();
	}

	//image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	//image.write_tga_file("output.tga");

	delete camera;
	delete[]zbuffer;
	delete[]framebuffer;
	window_destroy();

	return 0;
}
