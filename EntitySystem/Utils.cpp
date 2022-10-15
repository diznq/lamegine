#include "shared.h"

float frac(float n) {
	/*if (n == 1.0f) return 1.0f;
	float integral;
	return modf(n, &integral);*/
	return n;
}

void generateTangents(std::vector<Vertex>& vertices, std::vector<Index_t>& indices) {
	for (auto& it : vertices) {
		it.tangent = Vec3(0.0f, 0.0f, 0.0f);
	}
	int lim = (int)vertices.size();
	for (size_t i = 0; i < indices.size(); i += 3) {
		Vertex& v0 = vertices[indices[i]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		Vec3 Edge1 = v1.pos - v0.pos;
		Vec3 Edge2 = v2.pos - v0.pos;

		float DeltaU1 = frac(v1.texcoord.x) - frac(v0.texcoord.x);
		float DeltaV1 = frac(v1.texcoord.y) - frac(v0.texcoord.y);
		float DeltaU2 = frac(v2.texcoord.x) - frac(v0.texcoord.x);
		float DeltaV2 = frac(v2.texcoord.y) - frac(v0.texcoord.y);

		float f = 1.0f / (DeltaU1 * DeltaV2 - DeltaU2 * DeltaV1);

		Vec3 Tangent;

		Tangent.x = f * (DeltaV2 * Edge1.x - DeltaV1 * Edge2.x);
		Tangent.y = f * (DeltaV2 * Edge1.y - DeltaV1 * Edge2.y);
		Tangent.z = f * (DeltaV2 * Edge1.z - DeltaV1 * Edge2.z);

		//Bitangent.x = f * (-DeltaU2 * Edge1.x - DeltaU1 * Edge2.x);
		//Bitangent.y = f * (-DeltaU2 * Edge1.y - DeltaU1 * Edge2.y);
		//Bitangent.z = f * (-DeltaU2 * Edge1.z - DeltaU1 * Edge2.z);

		v0.tangent += Tangent;
		v1.tangent += Tangent;
		v2.tangent += Tangent;
	}
	for (auto& it : vertices) {
		it.tangent = it.tangent.normalize();
	}
}


const char *toHuman(float memUsage) {
	static char buffer[64];
	static char units[] = " kMGTP";
	int unit = 0;
	if (memUsage < 1024.0f) {
		sprintf(buffer, "%d B", (int)memUsage);
		return buffer;
	}
	while (memUsage > 1024.0f) {
		memUsage /= 1024.0f;
		unit++;
	}
	sprintf(buffer, "%.3f %cB", memUsage, units[unit]);
	return buffer;
}

std::string fixPath(std::string path) {
	for (char& c : path) {
		if (c == '\\') c = '/';
	}
	return path;
}
void fixSzPath(char *bf) {
	while (*bf) {
		if (*bf == '\\') *bf = '/';
		bf++;
	}
}


void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}