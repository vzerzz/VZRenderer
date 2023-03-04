#pragma once
#include "geometry.h"
#include "model.h"
#include "shader.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

void DrawCall(unsigned char* framebuffer, float *zbuffer,IShader& shader,int nface);
