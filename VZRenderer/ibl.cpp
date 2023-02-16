#include "ibl.h"
/* ibl prefilter */

// set normal vector for different face of cubemap
void set_normal_coord(int face_id, int x, int y, float& x_coord, float& y_coord, float& z_coord, float length = 255)
{
	switch (face_id)
	{
	case 0:   //positive x (right face)
		x_coord = 0.5f;
		y_coord = -0.5f + y / length;
		z_coord = -0.5f + x / length;
		break;
	case 1:   //negative x (left face)		
		x_coord = -0.5f;
		y_coord = -0.5f + y / length;
		z_coord = 0.5f - x / length;
		break;
	case 2:   //positive y (top face)
		x_coord = -0.5f + x / length;
		y_coord = 0.5f;
		z_coord = -0.5f + y / length;
		break;
	case 3:   //negative y (bottom face)
		x_coord = -0.5f + x / length;
		y_coord = -0.5f;
		z_coord = 0.5f - y / length;
		break;
	case 4:   //positive z (back face)
		x_coord = 0.5f - x / length;
		y_coord = -0.5f + y / length;
		z_coord = 0.5f;
		break;
	case 5:   //negative z (front face)
		x_coord = -0.5f + x / length;
		y_coord = -0.5f + y / length;
		z_coord = -0.5f;
		break;
	default:
		break;
	}
}

/* diffuse part */
// 漫反射的积分区域为一个半球 对所有的着色点以法线形成一个半球求半球的平均辐射度 预计算至irradiance_map中
void generate_irradiance_map(int face_id, TGAImage& image) {
	Texture* irradiancemap = new Texture("..\\..\\obj\\lake\\lake.obj", true);// 预处理所以单独提前读取处理
	for (int x = 0; x < 256; x++)
		for (int y = 0; y < 256; y++) {// irradiancemap 每个面大小为255*255
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord);
			Vec3f normal = (Vec3f(x_coord, y_coord, z_coord)).normalize();					 //z-axis
			Vec3f up = fabs(normal[1]) < 0.999f ? Vec3f(0.0f, 1.0f, 0.0f) : Vec3f(0.0f, 0.0f, 1.0f);
			Vec3f right = (cross(up, normal)).normalize();								 //tagent x-axis
			up = cross(normal, right);

			// 对漫反射方程的半球离散采样 
			Vec3f irradiance(0, 0, 0);
			float sampleDelta = 0.025;// 增量值
			float nrSamples = 0.0;
			for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
				for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
				{
					// spherical to cartesian (in tangent space)
					Vec3f tangentSample = Vec3f(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
					// tangent space to world
					Vec3f sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
					Vec3f color = irradiancemap->cubemap_sampling(sampleVec.normalize());
					irradiance += color * cos(theta) * sin(theta);
					nrSamples++;
				}
			irradiance = (float)PI * irradiance * (1.0f / float(nrSamples));// 平均
			for (int i = 0; i < 3; i++)
				irradiance[i] = (irradiance[i] < 255.f) ? irradiance[i] : 255.f;
			image.set(x, y, TGAColor(irradiance.x, irradiance.y, irradiance.z));
		}
}
// traverse all faces of cubemap for irradiance map
void foreach_irradiance_map()
{
	const char* faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];

	for (int j = 0; j < 6; j++) {
		sprintf_s(paths[j], "%s/i_%s.tga", "..\\..\\obj\\lake\\preprocessing", faces[j]);
	}
	TGAImage image = TGAImage(256, 256, TGAImage::RGB);
	for (int face_id = 0; face_id < 6; face_id++)
	{
		// 可用多线程加速
		generate_irradiance_map(face_id, image);

		image.flip_vertically(); // to place the origin in the bottom left corner of the image
		image.write_tga_file(paths[face_id]);
	}
}

/* specular part */
// split sum approximation

// 1. 预滤波环境贴图 预先计算对Li的积分 类似于辐照度图 考虑粗糙度 mipmap存储5个不同粗糙度值的预卷积结果
// 生成Van Der Corpus 序列 把十进制数字的二进制表示镜像翻转到小数点右边
float RadicalInverse_VdC(unsigned int bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// Hammersley 序列 随机的低差异序列 获取大小为 N 的样本集中的低差异样本 i
Vec2f Hammersley(unsigned int i, unsigned int N)
{
	return Vec2f(float(i) / float(N), RadicalInverse_VdC(i));
}

// 重要性采样 构建采样向量 使用 GGX NDF 定向和偏移采样向量，以使其朝向特定粗糙度的镜面波瓣方向
Vec3f ImportanceSampleGGX(Vec2f Xi, Vec3f N, float roughness)
{
	float a = roughness * roughness;// 平方粗糙度可以获得更好的视觉效果

	// 重要性采样 Inverse Transform Sampling 对一个均匀分布的序列U（0，1）做逆CDF，从而生成符合概率密度PDF分布的新序列
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	Vec3f H;
	H[0] = cos(phi) * sinTheta;
	H[1] = sin(phi) * sinTheta;
	H[2] = cosTheta;

	// from tangent-space vector to world-space sample vector
	Vec3f up = abs(N.z) < 0.999 ? Vec3f(0.0, 0.0, 1.0) : Vec3f(1.0, 0.0, 0.0);
	Vec3f tangent = (cross(up, N)).normalize();
	Vec3f bitangent = cross(N, tangent);

	Vec3f sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return sampleVec.normalize();
}

void generate_prefilter_map(int face_id, int mip_level, TGAImage& image)
{
	Texture* prefiltermap = new Texture("..\\..\\obj\\lake\\lake.obj", true);// 预处理所以单独提前读取处理
	// mipmap 大小
	int	factor = pow(2, mip_level);
	int width = 512 / factor;
	int height = 512 / factor;
	if (width < 64) width = 64;
	if (height < 64) height = 64;
	// roughness 层级
	float roughness[10];
	for (int i = 0; i < 10; i++)
		roughness[i] = i * (1.0 / 9.0);

	for (int x = 0; x < height; x++)
		for (int y = 0; y < width; y++)
		{
			float x_coord, y_coord, z_coord;
			set_normal_coord(face_id, x, y, x_coord, y_coord, z_coord, float(width - 1));

			Vec3f normal = Vec3f(x_coord, y_coord, z_coord);
			normal = normal.normalize();					//z-axis
			Vec3f up = fabs(normal[1]) < 0.999f ? Vec3f(0.0f, 1.0f, 0.0f) : Vec3f(0.0f, 0.0f, 1.0f);
			Vec3f right = (cross(up, normal)).normalize();	//x-axis
			up = cross(normal, right);						//y-axis

			Vec3f r = normal;
			Vec3f v = r;

			Vec3f prefilter_color = Vec3f(0, 0, 0);
			float total_weight = 0.0f;
			int numSamples = 1024;
			for (int i = 0; i < numSamples; i++)
			{
				Vec2f Xi = Hammersley(i, numSamples);
				Vec3f h = ImportanceSampleGGX(Xi, normal, roughness[mip_level]);
				Vec3f l = (2.0f * dot(v, h) * h - v).normalize();

				float n_dot_l = (dot(normal, l) > 0.f) ? dot(normal, l) : 0.f;

				if (n_dot_l > 0)
				{
					prefilter_color += prefiltermap->cubemap_sampling(l) * n_dot_l;
					total_weight += n_dot_l;
				}
			}

			prefilter_color = prefilter_color * (1.f / total_weight);
			for (int i = 0; i < 3; i++)
				prefilter_color[i] = (prefilter_color[i] < 255.f) ? prefilter_color[i] : 255.f;

			image.set(x, y, TGAColor(prefilter_color.x, prefilter_color.y, prefilter_color.z));
		}
}

// traverse all mipmap level for prefilter map
void foreach_prefilter_miplevel()
{
	const char* faces[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	char paths[6][256];

	for (int mip_level = 0; mip_level < 10; mip_level++)
	{
		for (int j = 0; j < 6; j++) {
			sprintf_s(paths[j], "%s/m%d_%s.tga", "..\\..\\obj\\lake\\preprocessing", mip_level, faces[j]);
		}
		int	factor = pow(2, mip_level);
		int width = 512 / factor;
		int height = 512 / factor;
		if (width < 64) width = 64;
		if (height < 64) height = 64;

		TGAImage image = TGAImage(width, height, TGAImage::RGB);
		for (int face_id = 0; face_id < 6; face_id++)
		{
			generate_prefilter_map(face_id, mip_level, image);
			//calculate_BRDF_LUT();
			image.flip_vertically(); // to place the origin in the bottom left corner of the image
			image.write_tga_file(paths[face_id]);
		}
	}

}

// 2. BRDF 积分贴图 lut part
float GeometrySchlickGGX_ibl(float ndotv, float roughness) {
	float k = (roughness * roughness) / 2.0f;
	return ndotv / (ndotv * (1 - k) + k);
}
float GeometrySmith(float ndotv, float ndotl, float roughness) {
	float ggx1 = GeometrySchlickGGX_ibl(ndotl, roughness);
	float ggx2 = GeometrySchlickGGX_ibl(ndotv, roughness);
	return ggx1 * ggx2;
}

Vec3f IntegrateBRDF(float NdotV, float roughness)
{
	// 由于各向同性，随意取一个 V 即可
	Vec3f V;
	V[0] = 0;
	V[1] = sqrt(1.0 - NdotV * NdotV);
	V[2] = NdotV;

	float A = 0.0;
	float B = 0.0;
	float C = 0.0;

	Vec3f N = Vec3f(0.0, 0.0, 1.0);

	const int SAMPLE_COUNT = 1024;
	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		// generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
		Vec2f Xi = Hammersley(i, SAMPLE_COUNT);
		Vec3f H = ImportanceSampleGGX(Xi, N, roughness);
		Vec3f L = (2.f * dot(V, H) * H - V).normalize();

		float NdotL = (L.z > 0.f) ? L.z : 0.f;
		float NdotV = (V.z > 0.f) ? V.z : 0.f;
		float NdotH = (H.z > 0.f) ? H.z : 0.f;
		float VdotH = (dot(V, H) > 0.f) ? dot(V, H) : 0.f;

		if (NdotL > 0.0)
		{
			float G = GeometrySmith(NdotV, NdotL, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);

			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}

	}

	return Vec3f(A, B, C) * (1.f / float(SAMPLE_COUNT));
}

/* traverse all 2d coord for lut part */
void BRDF_LUT()
{
	TGAImage image = TGAImage(255, 255, TGAImage::RGB);
	for (int i = 0; i < 256; i++)
		for (int j = 0; j < 256; j++)
		{
			Vec3f color;
			if (i == 0)
				color = IntegrateBRDF(0.002f, j / 256.0f);
			else
				color = IntegrateBRDF(i / 256.0f, j / 256.0f);
			for (int k = 0; k < 3; k++)
				color[k] = (color[k]*255.f < 255.f) ? color[k]*255.f : 255.f;

			image.set(i, j, TGAColor(color.x, color.y, color.z));
		}
	image.flip_vertically(); // to place the origin in the bottom left corner of the image
	image.write_tga_file( "..\\..\\obj\\lake\\preprocessing\\BRDF_LUT.tga");
}

