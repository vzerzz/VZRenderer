#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"


Model::Model(const char* filename, bool isskybox) : skybox(isskybox), verts_(), faces_(), norms_(), uvs_() {
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail()) {
		printf("load model failed\n");
		return;
	}
	std::string line;
	while (!in.eof()) {
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;
			Vec3f v;
			for (int i = 0; i < 3; i++) iss >> v[i];
			verts_.push_back(v);
		}
		else if (!line.compare(0, 3, "vn ")) {
			iss >> trash >> trash;
			Vec3f n;
			for (int i = 0; i < 3; i++) iss >> n[i];
			norms_.push_back(n);
		}
		else if (!line.compare(0, 3, "vt ")) {
			iss >> trash >> trash;
			Vec2f uv;
			for (int i = 0; i < 2; i++) iss >> uv[i];
			uvs_.push_back(uv);
		}
		else if (!line.compare(0, 2, "f ")) {
			std::vector<Vec3i> f;
			Vec3i tmp;
			iss >> trash;
			while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
				for (int i = 0; i < 3; i++) tmp[i]--; // in wavefront obj all indices start at 1, not zero
				f.push_back(tmp);
			}
			faces_.push_back(f);
		}
	}
	//std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << " vt# " << uvs_.size() << " vn# " << norms_.size() << std::endl;
	texture = new Texture(filename, isskybox);

}

int Model::nverts() {
	return (int)verts_.size();
}

int Model::nfaces() {
	return (int)faces_.size();
}

Vec3f Model::vert(int i) {
	return verts_[i];
}

Vec3f Model::vert(int iface, int nthvert)
{
	return verts_[faces_[iface][nthvert][0]];
}

Vec2f Model::uv(int iface, int nthvert) {
	return uvs_[faces_[iface][nthvert][1]];
}

Vec3f Model::normal(int iface, int nthvert)
{
	return norms_[faces_[iface][nthvert][2]];
}

std::vector<int> Model::face(int idx) {
	std::vector<int> face;
	for (int i = 0; i < (int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
	return face;
}
