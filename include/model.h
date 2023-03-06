#pragma once
#include <vector>
#include "geometry.h"
#include "tgaimage.h"
#include "texture.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<std::vector<Vec3i>> faces_; // attention, this Vec3i means vertex/uv/normal
	std::vector<Vec3f> norms_;
	std::vector<Vec2f> uvs_;

public:
	Model(const char *filename, bool isskybox);
	bool skybox;

	int nverts();
	int nfaces();
	// ����obj�д洢������
	Vec3f vert(int i);
	Vec3f vert(int iface, int nthvert);
	Vec2f uv(int iface, int nvert);
	Vec3f normal(int iface, int nthvert);
	std::vector<int> face(int idx);
	// texture��ͨ��uv�����������Ϊ���� ��sampling��
	//std::unique_ptr<Texture> texture;
	Texture* texture;
};