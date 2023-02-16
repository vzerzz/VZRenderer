#include "rasterizer.h"

//---------------------clip---------------------------
#define MAX_VERTEX 10

typedef enum {
	W_PLANE,
	X_RIGHT,
	X_LEFT,
	Y_TOP,
	Y_BOTTOM,
	Z_NEAR,
	Z_FAR
} clip_plane;

typedef struct {// 用于管理调度输入输出的顶点数据(两边倒)
	Vec3f in_world[MAX_VERTEX];
	Vec4f in_clip[MAX_VERTEX];
	Vec2f in_texture[MAX_VERTEX];
	Vec3f in_normal[MAX_VERTEX];
	Vec3f out_world[MAX_VERTEX];
	Vec4f out_clip[MAX_VERTEX];
	Vec2f out_texture[MAX_VERTEX];
	Vec3f out_normal[MAX_VERTEX];
}pass_attri_t;

pass_attri_t pass_attri;

bool is_inside_plane(clip_plane plane, Vec4f vertex) {// 判断是否在视锥内
	switch (plane)
	{
		case W_PLANE:
			return vertex.w <= -EPSILON;
		case X_RIGHT:
			return vertex.x >= vertex.w;
		case X_LEFT:
			return vertex.x <= -vertex.w;
		case Y_TOP:
			return vertex.y >= vertex.w;
		case Y_BOTTOM:
			return vertex.y <= -vertex.w;
		case Z_NEAR:
			return vertex.z >= vertex.w;
		case Z_FAR:
			return vertex.z <= -vertex.w;
		default:
			return 0;
	}
}

float get_intersect_ratio(Vec4f prev, Vec4f curv,clip_plane plane)// 求出插值系数
{
	switch (plane) 
	{
		case W_PLANE:
			return (prev.w + EPSILON) / (prev.w - curv.w);
		case X_RIGHT:
			return (prev.w - prev.x) / ((prev.w - prev.x) - (curv.w - curv.x));
		case X_LEFT:
			return (prev.w + prev.x) / ((prev.w + prev.x) - (curv.w + curv.x));
		case Y_TOP:
			return (prev.w - prev.y) / ((prev.w - prev.y) - (curv.w - curv.y));
		case Y_BOTTOM:
			return (prev.w + prev.y) / ((prev.w + prev.y) - (curv.w + curv.y));
		case Z_NEAR:
			return (prev.w - prev.z) / ((prev.w - prev.z) - (curv.w - curv.z));
		case Z_FAR:
			return (prev.w + prev.z) / ((prev.w + prev.z) - (curv.w + curv.z));
		default:
			return 0;
	}
}

int plane_clipping(clip_plane plane, int num_vert) {//对单个平面的裁剪
	int out_vert = 0;
	int pre_index, cur_index;
	bool rank = (plane + 1) % 2;
	Vec3f* in_world = rank ? pass_attri.in_world : pass_attri.out_world;
	Vec4f* in_clip = rank ? pass_attri.in_clip : pass_attri.out_clip;
	Vec2f* in_texture = rank ? pass_attri.in_texture : pass_attri.out_texture;
	Vec3f* in_normal = rank ? pass_attri.in_normal : pass_attri.out_normal;
	Vec3f* out_world = rank ? pass_attri.out_world : pass_attri.in_world;
	Vec4f* out_clip = rank ? pass_attri.out_clip : pass_attri.in_clip;
	Vec2f* out_texture = rank ? pass_attri.out_texture : pass_attri.in_texture;
	Vec3f* out_normal = rank ? pass_attri.out_normal : pass_attri.in_normal;

	for (int i = 0; i < num_vert; i++) {
		cur_index = i;
		pre_index = (i - 1 + num_vert) % num_vert;
		Vec4f cur_vertex = in_clip[cur_index];
		Vec4f pre_vertex = in_clip[pre_index];
		if (is_inside_plane(plane, cur_vertex) != is_inside_plane(plane, pre_vertex)) {//两个顶点不在一边则切割
			float ratio = get_intersect_ratio(pre_vertex, cur_vertex, plane);
			out_clip[out_vert] = pre_vertex + (cur_vertex - pre_vertex) * ratio;
			out_world[out_vert] = in_world[pre_index] + (in_world[cur_index] - in_world[pre_index]) * ratio;
			out_normal[out_vert] = in_normal[pre_index] + (in_normal[cur_index] - in_normal[pre_index]) * ratio;
			out_texture[out_vert] = in_texture[pre_index] + (in_texture[cur_index] - in_texture[pre_index]) * ratio;
			out_vert++;
		}
		if (is_inside_plane(plane, cur_vertex)) {//在内部的仍保留
			out_clip[out_vert] = cur_vertex;
			out_world[out_vert] = in_world[cur_index];
			out_normal[out_vert] = in_normal[cur_index];
			out_texture[out_vert] = in_texture[cur_index];
			out_vert++;
		}

	}
	return out_vert;
}

int homo_clipping() {//对七个平面裁剪
	int num_vert = 3;
	num_vert = plane_clipping(W_PLANE, num_vert);//裁剪掉w小于等于0 的
	num_vert = plane_clipping(X_RIGHT, num_vert);
	num_vert = plane_clipping(X_LEFT, num_vert);
	num_vert = plane_clipping(Y_TOP, num_vert);
	num_vert = plane_clipping(Y_BOTTOM, num_vert);
	num_vert = plane_clipping(Z_NEAR, num_vert);
	num_vert = plane_clipping(Z_FAR, num_vert);
	return num_vert;
}

//----------------------clip-------------------------------------


Vec3f barycentric(Vec3f* pts, Vec2f p) {
	Vec3f b = cross(Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - p.x), Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - p.y));
	if (std::abs(b.z) < 1) return Vec3f(-1, 1, 1);// 点在边上
	return Vec3f(1.f - (b.x + b.y) / b.z, b.x / b.z, b.y / b.z);
}


void rasterize(unsigned char* framebuffer, Vec4f* clip_coords, float* zbuffer, IShader& shader)
{
	Vec3f ndc_coords[3];
	Vec3f screen_coords[3];
	bool skybox = shader.payload.model->skybox;

	// 透视除法
	for (int i = 0; i < 3; i++)
	{
		ndc_coords[i][0] = clip_coords[i][0] / clip_coords[i].w;
		ndc_coords[i][1] = clip_coords[i][1] / clip_coords[i].w;
		ndc_coords[i][2] = clip_coords[i][2] / clip_coords[i].w;
	}

	// viewport
	for (int i = 0; i < 3; i++)
	{
		screen_coords[i][0] = 0.5 * (WINDOW_WIDTH - 1) * (ndc_coords[i][0] + 1);
		screen_coords[i][1] = 0.5 * (WINDOW_HEIGHT - 1) * (ndc_coords[i][1] + 1);
		screen_coords[i][2] = skybox ? 1000 : -clip_coords[i].w;// clip中w=z ndc中z为1 无价值
	}
	// backface clip
	if (!skybox) {
		float backface = ndc_coords[0].x * ndc_coords[1].y - ndc_coords[0].y * ndc_coords[1].x + ndc_coords[1].x * ndc_coords[2].y - ndc_coords[1].y * ndc_coords[2].x + ndc_coords[2].x * ndc_coords[0].y - ndc_coords[2].y * ndc_coords[0].x;
		if (backface <= 0) return;
	}

	// bounding box
	Vec2f bboxmin(WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1);
	Vec2f bboxmax(0, 0);
	Vec2f clamp(WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1);
	for (int i = 0; i < 3; i++) {
		bboxmin.x = std::max(0.f, std::min(bboxmin.x, screen_coords[i].x));
		bboxmin.y = std::max(0.f, std::min(bboxmin.y, screen_coords[i].y));
		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, screen_coords[i].x));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, screen_coords[i].y));
	}

	// rasterization
	for (int x = bboxmin.x; x <= bboxmax.x; x++)
		for (int y = bboxmin.y; y <= bboxmax.y; y++) {
			Vec3f bc_screen = barycentric(screen_coords, Vec2f(x+0.5f, y+0.5f));
			float alpha = bc_screen.x; float beta = bc_screen.y; float gamma = bc_screen.z;
			// inside triangle
			if (alpha < -EPSILON || beta < -EPSILON || gamma < -EPSILON) continue;
			
			float Z = 1.f / (alpha / clip_coords[0].w + beta / clip_coords[1].w + gamma / clip_coords[2].w);
			float z = (screen_coords[0].z * (alpha / clip_coords[0].w) + screen_coords[1].z * (beta / clip_coords[1].w) + screen_coords[2].z * (gamma / clip_coords[2].w)) * Z;
			int index = (WINDOW_HEIGHT - y - 1) * WINDOW_WIDTH + x;
			if (z < zbuffer[index]) {
				zbuffer[index] = z;
				
				Vec3f color = shader.fragment_shader(alpha, beta, gamma);
				int index = ((WINDOW_HEIGHT - y - 1) * WINDOW_WIDTH + x) * 4; // the origin for pixel is bottom-left, but the framebuffer index counts from top-left
				for (int i = 0; i < 3; i++)
					framebuffer[index + i] = color[i];
				//image.set(x, y, TGAColor(color[0], color[1], color[2]));
			}
		}
}

void draw(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface)
{
	for (int i = 0; i < 3; i++) {
		shader.vertex_shader(nface, i);// vertex shader
		pass_attri.in_clip[i] = shader.payload.clip_coords[i];
		pass_attri.in_normal[i] = shader.payload.normals[i];
		pass_attri.in_texture[i] = shader.payload.texture_coords[i];
		pass_attri.in_world[i] = shader.payload.world_coords[i];
	}
	// homogeneous clipping
	int num_vert = homo_clipping();// 返回三角面经切割后的顶点数
	// obj文件中面要均为三角面 (;_;)
	for (int i = 0; i < num_vert - 2; i++) {//顶点数一般为3或4 3则直接赋值 4分割成两个三角面再光栅化
		shader.payload.clip_coords[0] = pass_attri.out_clip[0];
		shader.payload.clip_coords[1] = pass_attri.out_clip[i+1];
		shader.payload.clip_coords[2] = pass_attri.out_clip[i+2];
		shader.payload.world_coords[0] = pass_attri.out_world[0];
		shader.payload.world_coords[1] = pass_attri.out_world[i+1];
		shader.payload.world_coords[2] = pass_attri.out_world[i+2];
		shader.payload.normals[0] = pass_attri.out_normal[0];
		shader.payload.normals[1] = pass_attri.out_normal[i+1];
		shader.payload.normals[2] = pass_attri.out_normal[i+2];
		shader.payload.texture_coords[0] = pass_attri.out_texture[0];
		shader.payload.texture_coords[1] = pass_attri.out_texture[i+1];
		shader.payload.texture_coords[2] = pass_attri.out_texture[i+2];

		rasterize(framebuffer, shader.payload.clip_coords, zbuffer, shader);
	}

}

/*void drawLine(Vec2i u, Vec2i v, TGAImage& image, TGAColor color) {
	bool steap = false;
	if (std::abs(v.x - u.x) < std::abs(v.y - u.y)) {
		std::swap(u.x, u.y);
		std::swap(v.x, v.y);
		steap = true;
	}
	if (v.x - u.x < 0) {
		std::swap(u.x, v.x);
		std::swap(u.y, v.y);
	}
	int d = 0;
	for (int x = u.x, y = u.y; x <= v.x; x++) {
		if (steap) image.set(y, x, color);
		else image.set(x, y, color);
		d += 2 * std::abs(u.y - v.y);
		if (d > v.x - u.x) {
			d -= 2 * (v.x - u.x);
			y += (v.y > u.y ? 1 : -1);
		}
	}
}*/

