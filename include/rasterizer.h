#pragma once
#include "geometry.h"
#include "model.h"
#include "shader.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;



void rasterize(unsigned char* framebuffer, Vec4f* clip_coords, float* zbuffer, IShader& shader);
void draw(unsigned char* framebuffer, float *zbuffer,IShader& shader,int nface);
