#include "shader.h"

static Vec3f TBN_normal(Vec3f& normal, Vec3f* world_coords, Vec2f* texture_coords, Vec2f& texture_coord, Vec3f& uv_normal) {
	float uAB = texture_coords[1][0] - texture_coords[0][0];
	float vAB = texture_coords[1][1] - texture_coords[0][1];
	float uAC = texture_coords[2][0] - texture_coords[0][0];
	float vAC = texture_coords[2][1] - texture_coords[0][1];
	Vec3f AB = world_coords[1] - world_coords[0];
	Vec3f AC = world_coords[2] - world_coords[0];
	Vec3f t = (AB * vAC - AC * vAB) * (1 / (uAB * vAC - uAC * vAB));
	Vec3f b = (AB * (-uAC) + AC * uAB) * (1 / (uAB * vAC - uAC * vAB));
	// schmidt orthogonalization
	Vec3f n = normal.normalize();
	t = (t - dot(t, n) * n).normalize();
	b = (b - dot(b, n) * n - dot(b, t) * t).normalize();
	Vec3f res = t * uv_normal[0] + b * uv_normal[1] + n * uv_normal[2];
	return res;

}

// Trowbridge-Reitz GGX		NDF		return [0,1] 表示与半程向量h取向一致的微平面的比率
float DistributionGGX(Vec3f n, Vec3f h, float roughness) {
	float alpha = roughness * roughness;
	float ndoth = std::max(dot(n, h), 0.0f);
	float temp = ndoth * ndoth * (alpha * alpha - 1) + 1;
	float res = alpha * alpha / (PI * temp * temp);
	return res;
}

// Schlick-GGX		G		return [0,1] 1.0表示没有微平面阴影，0.0则表示微平面彻底被遮蔽
// k是roughness的重映射(Remapping)，取决于要用的是针对直接光照还是针对IBL光照的几何函数
float GeometrySchlickGGX(Vec3f n, Vec3f v, float roughness) {
	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0f;
	float ndotv = std::max(dot(n, v), 0.0f);
	float res = ndotv / (ndotv * (1 - k) + k);
	return res;
}
// Smith’s method 同时考虑观察方向（几何遮蔽(Geometry Obstruction)）和光线方向向量（几何阴影(Geometry Shadowing)）
float GeometrySmith(Vec3f n, Vec3f v, Vec3f l, float roughness) {
	float ggx1 = GeometrySchlickGGX(n, l, roughness);
	float ggx2 = GeometrySchlickGGX(n, v, roughness);
	return ggx1 * ggx2;
}

// Fresnel-Schlick		F		因观察角度与反射平面方向的夹角而引起的反射程度不同
// F0受金属工作流的影响 
Vec3f FresenlSchlick(Vec3f h, Vec3f v, Vec3f& F0) {
	float hdotv = std::max(dot(h, v), 0.0f);
	return F0 + (Vec3f(1.0f, 1.0f, 1.0f) - F0) * pow(1.0f - hdotv, 5.0f);
}
// Roughness 会引起反射率不同
Vec3f FresenlSchlickRoughness(Vec3f h, Vec3f v, Vec3f& F0, float roughness) {
	float hdotv = std::max(dot(h, v), 0.0f);
	float r = 1.f - roughness;
	if (r < F0[0]) r = F0[0];
	return F0 + (Vec3f(r, r, r) - F0) * pow(1.0f - hdotv, 5.0f);
}



void PBRShader::vertex_shader(int nfaces, int nvertex) {
	Vec3f vert_ = payload.model->vert(nfaces, nvertex);

	payload.world_coords[nvertex] = vert_;
	payload.normals[nvertex] = payload.model->normal(nfaces, nvertex);
	payload.texture_coords[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.clip_coords[nvertex] = payload.mvpMatrix * to_Vec4(vert_);
}

// for direct light
Vec3f PBRShader::fragment_shader_direct(float alpha, float beta, float gamma)
{
	light_t light[5];
	for (auto& l : light)
		l.radiance = Vec3f(1.f, 1.f, 1.f);
	light[0].position = Vec3f(3.f, 3.f, 3.f);
	light[1].position = Vec3f(-3.f, 3.f, -3.f);
	light[2].position = Vec3f(3.f, -3.f, 3.f);
	light[3].position = Vec3f(-3.f, -3.f, -3.f);
	light[4].position = Vec3f(3.f, 3.f, -3.f);

	// 插值 且矫正
	float Z = 1.f / (alpha / payload.clip_coords[0].w + beta / payload.clip_coords[1].w + gamma / payload.clip_coords[2].w);
	Vec3f world_coord = (payload.world_coords[0] * (alpha / payload.clip_coords[0].w) + payload.world_coords[1] * (beta / payload.clip_coords[1].w) + payload.world_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;
	Vec3f normal = (payload.normals[0] * (alpha / payload.clip_coords[0].w) + payload.normals[1] * (beta / payload.clip_coords[1].w) + payload.normals[2] * (gamma / payload.clip_coords[2].w)) * Z;
	Vec2f texture_coord = (payload.texture_coords[0] * (alpha / payload.clip_coords[0].w) + payload.texture_coords[1] * (beta / payload.clip_coords[1].w) + payload.texture_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;
	// 切线空间法线贴图
	if (payload.model->texture->normalmap_) {
		Vec3f uv_normal = payload.model->texture->normal_sampling(texture_coord);
		normal = TBN_normal(normal, payload.world_coords, payload.texture_coords, texture_coord, uv_normal);
	}

	// 对贴图采样
	float roughness = payload.model->texture->roughness_sampling(texture_coord);
	float metalness = payload.model->texture->metalness_sampling(texture_coord);
	float ao = payload.model->texture->occlusion_sampling(texture_coord);
	Vec3f albedo = payload.model->texture->diffuse_sampling(texture_coord);//金属表面颜色
	Vec3f emission = payload.model->texture->emission_sampling(texture_coord);
	albedo = Vec3f(std::pow(albedo[0], 2.2f), std::pow(albedo[1], 2.2f), std::pow(albedo[2], 2.2f));
	emission = Vec3f(std::pow(emission[0], 2.2f), std::pow(emission[1], 2.2f), std::pow(emission[2], 2.2f));

	Vec3f color(0, 0, 0);

	Vec3f n = normal.normalize();
	Vec3f v = (payload.camera->eye_pos - world_coord).normalize();
	Vec3f Lo = Vec3f(0.f, 0.f, 0.f);
	// 多个光源则对以下至求得Lo处for循环 相加得总Lo
	for (int i = 0; i < 5; i++) {
		Vec3f l = (light[i].position - world_coord).normalize();
		Vec3f h = (l + v).normalize();
		if (std::max(dot(n, l), 0.f) > 0) {
			float NDF = DistributionGGX(n, h, roughness);
			float G = GeometrySmith(n, v, l, roughness);
			// 金属流 F0会因为材料不同而不同且变色 认为大多数的绝缘体F0为0.04
			Vec3f tempF = Vec3f(0.04f, 0.04f, 0.04f);
			Vec3f F0 = tempF + (albedo - tempF) * metalness;
			Vec3f F = FresenlSchlick(h, v, F0);

			Vec3f Ks = F;
			Vec3f Kd = (Vec3f(1.f, 1.f, 1.f) - Ks) * (1.f - metalness);// 金属不会折射
			float ndotl = std::max(dot(n, l), 0.f);
			float ndotv = std::max(dot(n, v), 0.f);
			Vec3f CookTorrance_brdf = NDF * G * F * (1.f / (4.f * ndotl * ndotv + 0.0001f));// +0.0001防除0
			Lo += (Kd * albedo * (1 / PI) + CookTorrance_brdf) * light[i].radiance * ndotl;
		}
	}
	// 环境光项
	Vec3f ambient = Vec3f(0.03f, 0.03f, 0.03f) * albedo * ao;// ao 为环境光遮蔽
	color = ambient + Lo + emission;

	for (int i = 0; i < 3; i++) {
		// ACES ToneMapping
		const float A = 2.51f;
		const float B = 0.03f;
		const float C = 2.43f;
		const float D = 0.59f;
		const float E = 0.14f;
		color[i] = (color[i] * (A * color[i] + B)) / (color[i] * (C * color[i] + D) + E);
		color[i] = color[i] < 0 ? 0 : (color[i] > 1 ? 1 : color[i]);
		// Gamma Correction
		color[i] = pow(color[i], 1.f / 2.2f);
	}

	return color * 255.f;
}

Vec3f PBRShader::fragment_shader(float alpha, float beta, float gamma)
{
	// 插值 且矫正
	float Z = 1.f / (alpha / payload.clip_coords[0].w + beta / payload.clip_coords[1].w + gamma / payload.clip_coords[2].w);
	Vec3f world_coord = (payload.world_coords[0] * (alpha / payload.clip_coords[0].w) + payload.world_coords[1] * (beta / payload.clip_coords[1].w) + payload.world_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;
	Vec3f normal = (payload.normals[0] * (alpha / payload.clip_coords[0].w) + payload.normals[1] * (beta / payload.clip_coords[1].w) + payload.normals[2] * (gamma / payload.clip_coords[2].w)) * Z;
	Vec2f texture_coord = (payload.texture_coords[0] * (alpha / payload.clip_coords[0].w) + payload.texture_coords[1] * (beta / payload.clip_coords[1].w) + payload.texture_coords[2] * (gamma / payload.clip_coords[2].w)) * Z;
	// 切线空间法线贴图
	if (payload.model->texture->normalmap_) {
		Vec3f uv_normal = payload.model->texture->normal_sampling(texture_coord);
		normal = TBN_normal(normal, payload.world_coords, payload.texture_coords, texture_coord, uv_normal);
	}

	// 对贴图采样
	float roughness = payload.model->texture->roughness_sampling(texture_coord);
	float metalness = payload.model->texture->metalness_sampling(texture_coord);
	float ao = payload.model->texture->occlusion_sampling(texture_coord);
	Vec3f albedo = payload.model->texture->diffuse_sampling(texture_coord);//金属表面颜色
	Vec3f emission = payload.model->texture->emission_sampling(texture_coord);
	// Gamma Correction Removed
	// 将纹理都转为线性空间
	albedo = Vec3f(std::pow(albedo[0], 2.2f), std::pow(albedo[1], 2.2f), std::pow(albedo[2], 2.2f));
	emission = Vec3f(std::pow(emission[0], 2.2f), std::pow(emission[1], 2.2f), std::pow(emission[2], 2.2f));
	Vec3f color(0, 0, 0);

	Vec3f n = normal.normalize();
	Vec3f v = (payload.camera->eye_pos - world_coord).normalize();
	float ndotv = std::max(dot(n, v), 0.f);

	if (ndotv > 0) {
		// 金属流 F0会因为材料不同而不同且变色 认为大多数的绝缘体F0为0.04
		Vec3f tempF = Vec3f(0.04f, 0.04f, 0.04f);
		Vec3f F0 = tempF + (albedo - tempF) * metalness;
		Vec3f F = FresenlSchlickRoughness(n, v, F0, roughness);
		Vec3f Ks = F;
		Vec3f Kd = (Vec3f(1.f, 1.f, 1.f) - Ks) * (1.f - metalness);// 金属不会折射
	
		// diffuse part
		cubemap_t* irradiance_map = payload.iblmap->irradiance_map;
		Vec3f irradiance = cubemap_sampling(irradiance_map, n);
		Vec3f diffuse = irradiance * Kd * albedo;

		// specular part
		Vec3f l = (2.f*dot(v, n) * n - v).normalize();
		Vec2f lut_uv = Vec2f(ndotv, roughness);
		Vec3f lut_sample = texture_sampling(payload.iblmap->brdf_lut, lut_uv);
		float specular_scale = lut_sample.x;
		float specular_bias = lut_sample.y;
		Vec3f specular = F0 * specular_scale + Vec3f(specular_bias, specular_bias, specular_bias);
		float max_mip_level = (float)(payload.iblmap->mip_levels - 1);
		int specular_miplevel = (int)(roughness * max_mip_level + 0.5f);
		Vec3f prefilter_color = cubemap_sampling(payload.iblmap->prefilter_maps[specular_miplevel], l);
		specular = (prefilter_color * specular);

		color = (diffuse + specular) + emission;
	}

	for (int i = 0; i < 3; i++) {
		// ACES ToneMapping
		const float A = 2.51f;
		const float B = 0.03f;
		const float C = 2.43f;
		const float D = 0.59f;
		const float E = 0.14f;
		color[i] = (color[i] * (A * color[i] + B)) / (color[i] * (C * color[i] + D) + E);
		color[i] = color[i] < 0 ? 0 : (color[i] > 1 ? 1 : color[i]);
		// Gamma Correction
		color[i] = pow(color[i], 1.f / 2.2f);
	}

	return color * 255.f;
}

