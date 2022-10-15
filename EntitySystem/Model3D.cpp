#include "shared.h"
#include "Model3D.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include <map>
#include <fstream>
#include <queue>
#ifdef ALLOW_FBX_MODELS
#include <fbxsdk.h>
#pragma comment(lib, "libfbxsdk-mt")
#endif
#define ALLOW_ASSIMP
#ifdef ALLOW_ASSIMP
#undef min
#undef max
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma comment(lib, "assimp")
#pragma comment(lib, "IrrXML")
#pragma comment(lib, "zlibstatic")
#endif
#include "JSON.h"

#define FMT_VERSION 2

#ifdef ALLOW_ASSIMP
matrix FromAssimp(aiMatrix4x4& src) {
	matrix mtx;
	mtx._11 = src.a1; mtx._12 = src.a2; mtx._13 = src.a3; mtx._14 = src.a4;
	mtx._21 = src.b1; mtx._22 = src.b2; mtx._23 = src.b3; mtx._24 = src.b4;
	mtx._31 = src.c1; mtx._32 = src.c2; mtx._33 = src.c3; mtx._34 = src.c4;
	mtx._41 = src.d1; mtx._42 = src.d2; mtx._43 = src.d3; mtx._44 = src.d4;
	return mtx;
}
#endif

Model3D::~Model3D() {
	//printf("Removing %s\n", path.c_str());
}

IObject3D& Model3D::init(IEngine *pEngine) {
	Object3D::init(pEngine);
	std::string pathPrefix = "";
	path = fixPath(path);
	
	flags = ENT_GEN_OBJECT;
	type = DEFAULT_OBJECT;
	//setScale(1.0f);
	setPos(float3(0.0f, 0.0f, 0.0f));
	
	try {
		if (queryDrawData(((IEngine*)pEngine)->getDrawDataManager(), path.c_str())) {
			const char *ptr = path.c_str();
			const char *start = ptr;
			size_t last = -1;
			while (*ptr) {
				if ((*ptr == '\\' || *ptr == '/')) last = ptr - start;
				ptr++;
			}
			if (last != -1) {
				pathPrefix = std::string(path).substr(0, last + 1);
			}
			
			if (path.find(".lmm") != std::string::npos) {
				importModel(path, pathPrefix);
			} else if (path.find(".obj") != std::string::npos) {
				importObjModel(path, pathPrefix);
			} else {
#ifdef ALLOW_ASSIMP
				importOther(path, pathPrefix);
#endif
			}
		}// else printf("Model already existed!\n");
		//sw.stop("Model loaded");
	} catch (std::exception& ex) {
		//...
	}

	if (rootJoint && !animator) {
		animator = gcnew<Animator>(rootJoint);
		startAnimation(getAnimation("Unnamed"));
	}
	
	postInit();
	
	return *this;
}

bool Model3D::isAnimated() const {
	return animated;
}

void Model3D::importObjModel(std::string path, std::string pathPrefix) {
	int sv = 0, sn = 0, st = 0;
	int lastLine = 0;
	char line[255];
	bool unite = false;
	std::queue<std::string> objName;

	std::map<std::string, Ptr<IMaterial> > mtls;
	FILE *f = fopen(path.c_str(), "r");
	std::vector<float3> vts, nrm;
	std::vector<float2> uvs;
	int fCtr = 0;
	int mpv = 0;
	Ptr<IDrawData> pDD = getDrawData();
	std::vector<Vertex>& vertices = pDD->getVertices();
	vertices.reserve(10000);
	std::vector<Index_t>& indices = pDD->getIndices();
	auto& objects = pDD->getObjects();
	indices.reserve(10000);

	bool was_faces = false;

	std::shared_ptr<IMaterial> lastMtl;
	std::string lastObjName;

	float3 relPos = float3(0.0f, 0.0f, 0.0f);
	int relCount = 0;
	unsigned int lastObj = 0;

	IEngine *pEngine = engine;
	
	if (f && !vertices.size() && !indices.size()) {
		while (fgets(line, 255, f)) {
			lastLine++;
			char ctl[255];
			int len = sscanf(line, "%s", ctl);

			if (!strcmp(ctl, "mtllib")) {
				sscanf(line + 7, "%s", ctl);
				readMaterials((IEngine*)pEngine, pathPrefix, (pathPrefix + ctl).c_str(), mtls);
			}
			else if (!strcmp(ctl, "vn")) {
				float x, y, z;
				sscanf(line + 3, "%f %f %f", &x, &z, &y);
				nrm.push_back(float3(x, -y, z));
			}
			else if (!strcmp(ctl, "v")) {
				if (was_faces && !unite && relCount) {
					objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), lastMtl, relPos, objName.size() ? objName.front() : "Block"));
					//objName.pop();
					was_faces = false;
				}
				float x, y, z;
				sscanf(line + 2, "%f %f %f", &x, &z, &y);
				y = -y;
				x = x;
				y = y;
				z = z;
				float dst = sqrt(x *x + y * y + z * z);
				vts.push_back(float3(x, y, z));
				relPos += float3(x, y, z);
				relCount++;
			}
			else if (!strcmp(ctl, "vt")) {
				float u, v;
				sscanf(line + 3, "%f %f", &u, &v);
				uvs.push_back(float2(u, 1.0f - v));
			}
			else if (!strcmp(ctl, "f")) {
				was_faces = true;
				std::string part;
				std::stringstream ss;
				ss << (line + 2);
				//ss << (line + 2);
				int indstart = fCtr;
				int loaded = 0;

				while ( ss >> part ) {
					char str[64];
					strcpy(str, part.c_str());
					int v, t, n;
					int c = sscanf(str, "%d/%d/%d", &v, &t, &n);
					if (c == 1) {
						if (sscanf(str, "%d//%d", &v, &n) == 2) {
							c = 3;
							t = 0;
						}
					}
					if (c == 3) {
						v--; t--; n--;
						Vertex vtx;
						if (v - sv < 0 || v - sv >= (int)vts.size()) continue;
						vtx.pos = vts[v - sv];
						if (n - sn < 0 || n - sn >= (int)nrm.size()) continue;
						vtx.normal = nrm[n - sn];
						vtx.jointIds = uint3(-1, -1, -1);
						if (uvs.size() && (t - st) >= 0 && (t - st) < (int)uvs.size()) {
							vtx.texcoord = uvs[t - st];
						}
						vertices.push_back(vtx);
						if (loaded < 3) {
							indices.push_back((Index_t)fCtr++);
						}
						else {
							indices.push_back((Index_t)fCtr - 3);
							indices.push_back((Index_t)fCtr - 1);
							indices.push_back((Index_t)fCtr);
							fCtr++;
						}
						loaded++;
					}
				}
				if (loaded > mpv) mpv = loaded;
			}
			else if (!strcmp(ctl, "usemtl")) {
				char mtl[255];
				sscanf(line + 7, "%s", mtl);
				if (indices.size()) {
					objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), lastMtl, relPos, objName.size() ? objName.front() : "Block"));
					if (objName.size())
						objName.pop();
				}
				std::string idx = mtl;
				if (mtls.find(idx) != mtls.end()) lastMtl = mtls[idx];
			}
			else if (!strcmp(ctl, "o")) {
				if (was_faces && !unite && relCount) {
					objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), lastMtl, relPos, objName.size() ? objName.front() : "Block"));
					//objName.pop();
					was_faces = false;
				}
				for (unsigned int i = lastObj; i < objects.size(); i++) {
					objects[i]->name = lastObjName;
				}
				lastObj = (unsigned int)objects.size();
				char nm[255];
				sscanf(line + 2, "%s", nm);
				lastObjName = nm;
				objName.push(nm);
			}
		}

		// sw.stop("Loading model");

		for (size_t i = 0; i < indices.size(); i += 3) {
			int i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
			indices[i] = i2;
			indices[i + 1] = i0;
			indices[i + 2] = i1;
		}

		vertices.shrink_to_fit();
		indices.shrink_to_fit();

		generateTangents(vertices, indices);

		std::string objectName = "Generic";
		if (objName.size()) objectName = objName.front();

		objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), lastMtl, float3(0.0f, 0.0f, 0.0f), objectName));
		for (unsigned int i = lastObj; i < objects.size(); i++) {
			objects[i]->name = lastObjName;
		}

		//printf("Model memory usage: %s\n", getHumanMemoryUsage());
		//printf("Vertices: %zu (x%zu = %zu)\n", vertices.size(), sizeof(Vertex), sizeof(Vertex) * vertices.size());
		//printf("Indices: %zu (x%zu = %zu)\n", indices.size(), sizeof(Index_t), sizeof(Index_t) * indices.size());
	}
	pDD->finish();
	fclose(f);
}

void Model3D::importOther(std::string path, std::string pathPrefix) {
#ifdef ALLOW_ASSIMP
	if (getDrawData()->getVertices().size() || getDrawData()->getIndices().size()) return;
	Assimp::Importer importer;
	const aiScene* pScene = importer.ReadFile(path,
		aiProcess_Triangulate
		| aiProcess_CalcTangentSpace
		| aiProcess_OptimizeGraph
		| aiProcess_OptimizeMeshes
	);
	if (pScene) {
		rootJoint = nullptr;
		importAssimpNode(pathPrefix, rootJoint, pScene, pScene->mRootNode);
		if (rootJoint) {
			rootJoint->printHierarchy(nullptr, 0);
			matrix parentMatrix(1.0f);
			rootJoint->calcInverseBindTransform(parentMatrix);
			animated = true;

			std::map<int, float4> bonePositions;

			for (auto& vtx : getDrawData()->getVertices()) {
				bonePositions[vtx.jointIds.x] += float4(vtx.pos.x, vtx.pos.y, vtx.pos.z, 1.0f);
				bonePositions[vtx.jointIds.y] += float4(vtx.pos.x, vtx.pos.y, vtx.pos.z, 1.0f);
				bonePositions[vtx.jointIds.z] += float4(vtx.pos.x, vtx.pos.y, vtx.pos.z, 1.0f);
			}

			for (auto it : bonePositions) {
				if (it.first != -1) {
					auto bone = boneHierarchy[revMapping[it.first]];
					float3 pos(it.second.x / it.second.w, it.second.y / it.second.w, it.second.z / it.second.w);
					bone->setPosition(pos);
				}
			}

			for (unsigned int i = 0; i < pScene->mNumAnimations; i++) {
				auto& anim = pScene->mAnimations[i];
				std::string animName = "Unnamed";
				if (anim && anim->mName.length) animName = anim->mName.C_Str();
			
				float dur = (float)(anim->mDuration / anim->mTicksPerSecond);
				std::vector<Ptr<IKeyFrame>> keyframes;
				for (unsigned int j = 0; j < anim->mNumChannels; j++) {
					auto& chan = anim->mChannels[j];
					if (keyframes.size() == 0) keyframes.resize(chan->mNumRotationKeys);
					for (unsigned int k = 0; k < chan->mNumRotationKeys; k++) {
						if (j == 0) {
							Ptr<IKeyFrame> keyFrame = gcnew<KeyFrame>((float)chan->mRotationKeys[k].mTime, std::unordered_map<std::string, JointTransform>());
							keyframes[k] = keyFrame;
						}
						auto& kf = keyframes[k];
						auto& jt = kf->getJointTransforms()[std::string(chan->mNodeName.C_Str())];
						auto& pos = chan->mPositionKeys[k].mValue;
						auto& rot = chan->mRotationKeys[k].mValue;
						jt.position = float3((float)pos.x, (float)pos.y, (float)pos.z);
						jt.rotation = float4((float)rot.x, (float)rot.y, (float)rot.z, (float)rot.w);

					}
				}
				Ptr<Animation> pAnimation = gcnew<Animation>(dur, keyframes);
				animations[animName] = pAnimation;
			}
		}
	}
	auto& indices = getDrawData()->getIndices();
	auto& vertices = getDrawData()->getVertices();

	generateTangents(vertices, indices);

	getDrawData()->finish();
#endif
}

void Model3D::importAssimpNode(std::string pathPrefix, Ptr<IJoint> root, const void *s, void *n){
#ifdef ALLOW_ASSIMP
	aiNode *node = (aiNode*)n;
	const aiScene *scene = (const aiScene*)s;
	if (!node) {
		return;
	}
	auto& vertices = getDrawData()->getVertices();
	auto& indices = getDrawData()->getIndices();
	auto& objects = getDrawData()->getObjects();
	const char *parent = "None";
	if (node->mParent && node->mParent->mName.C_Str()) parent = node->mParent->mName.C_Str();
	std::string currentName(node->mName.C_Str());
	auto srch = boneHierarchy.find(currentName);
	Ptr<IJoint> jt;
	if (srch != boneHierarchy.end()) {
		jt = srch->second;
	} else {
		jt = gcnew<Joint>(0, currentName, matrix(1.0f));
		boneHierarchy[std::string(node->mName.C_Str())] = jt;
	}
	if (node->mParent) {
		auto it = boneHierarchy.find(std::string(node->mParent->mName.C_Str()));
		if (it != boneHierarchy.end()) {
			it->second->addChild(boneHierarchy[std::string(node->mName.C_Str())]);
		}
	}
	//cout << "Importing " << node->mName.C_Str() << ", parent: " << parent << endl;
	for (UINT k = 0; k < node->mNumMeshes; k++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[k]];
		IEngine *pEngine = engine;
		Ptr<IMaterial> mtl = pEngine->getRenderer()->createMaterial("ImportedMaterial");
		mtl->add(pEngine->getDefaultAlpha());
		mtl->add(pEngine->getDefaultTexture());
		mtl->add(pEngine->getDefaultDiffuse());
		mtl->add(pEngine->getDefaultBump());
		mtl->add(pEngine->getDefaultMisc());

		uint offset = (uint)vertices.size();
		for (UINT i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;

			vertex.pos.x = mesh->mVertices[i].x;
			vertex.pos.y = mesh->mVertices[i].y;
			vertex.pos.z = mesh->mVertices[i].z;

			vertex.jointIds = uint3(-1, -1, -1);

			if (mesh->mNormals) {
				vertex.normal.x = mesh->mNormals[i].x;
				vertex.normal.y = mesh->mNormals[i].y;
				vertex.normal.z = mesh->mNormals[i].z;
			}

			if (mesh->mTangents) {
				vertex.tangent.x = mesh->mTangents[i].x;
				vertex.tangent.y = mesh->mTangents[i].y;
				vertex.tangent.z = mesh->mTangents[i].z;
			}

			if (mesh->mTextureCoords[0])
			{
				vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
				vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
			}

			vertex.jointIds.x = -1;
			vertex.jointIds.y = -1;
			vertex.jointIds.z = -1;

			vertices.push_back(vertex);
		}

		for (UINT i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (UINT j = 0; j < face.mNumIndices; j++)
				indices.push_back(offset + face.mIndices[j]);
		}


#if 1
		for (UINT i = 0; i < mesh->mNumBones; i++) {

			uint jointIndex = 0;
			std::string boneName(mesh->mBones[i]->mName.data);
			
			if (jointMapping.find(boneName) == jointMapping.end()) {
				jointIndex = jointCount;
				revMapping[jointIndex] = boneName;
				jointCount++;
				jointTransforms.push_back(matrix(1.0f));
			} else {
				jointIndex = jointMapping[boneName];
			}
			
			jointMapping[boneName] = jointIndex;
			matrix mtx; auto& src = mesh->mBones[i]->mOffsetMatrix.Inverse();
			mtx = FromAssimp(src);
			jointTransforms[jointIndex] = mtx;

			auto it = boneHierarchy.find(boneName);
			if (it != boneHierarchy.end()) {
				it->second->reset(jointIndex, boneName, mtx);
				if (!rootJoint) {
					rootJoint = it->second;
				}
			}

			for (uint j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
				uint VertexID = offset + mesh->mBones[i]->mWeights[j].mVertexId;
				float Weight = mesh->mBones[i]->mWeights[j].mWeight;
				auto& vtx = vertices[VertexID];
				if (vtx.jointIds.x == -1) {
					vtx.jointIds.x = jointIndex;
					vtx.weights.x = Weight;
				} else if (vtx.jointIds.y == -1) {
					vtx.jointIds.y = jointIndex;
					vtx.weights.y = Weight;
				} else if (vtx.jointIds.z == -1) {
					vtx.jointIds.z = jointIndex;
					vtx.weights.z = Weight;
				}
			}
		}
#endif

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			aiString txPath;
			
			if (material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &txPath) == aiReturn_SUCCESS) {
				mtl->add(engine->loadTexture((pathPrefix + txPath.C_Str()).c_str(), "Diffuse", true, TextureSlotDiffuse));
			}
			if (material->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &txPath) == aiReturn_SUCCESS) {
				mtl->add(engine->loadTexture((pathPrefix + txPath.C_Str()).c_str(), "Normal", true, TextureSlotNormal));
			}
			if (material->GetTexture(aiTextureType::aiTextureType_SPECULAR, 0, &txPath) == aiReturn_SUCCESS) {
				mtl->add(engine->loadTexture((pathPrefix + txPath.C_Str()).c_str(), "Specular", true, TextureSlotMisc));
			}
			if (material->GetTexture(aiTextureType::aiTextureType_AMBIENT, 0, &txPath) == aiReturn_SUCCESS) {
				mtl->add(engine->loadTexture((pathPrefix + txPath.C_Str()).c_str(), "Ambient", true, TextureSlotAmbient));
			}
			
			//vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
			//textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		}

		objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), mtl, float3(0.0f, 0.0f, 0.0f), mesh->mName.C_Str()));
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		this->importAssimpNode(pathPrefix, root, scene, node->mChildren[i]);
	}
#endif
}

#ifdef ALLOW_FBX_MODELS
FbxVector4 GetFBXNormal(FbxGeometryElementNormal* normalElement, int polyIndex, int posIndex) {
	if (normalElement) {
		if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
			if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				return normalElement->GetDirectArray().GetAt(posIndex);
			int i = normalElement->GetIndexArray().GetAt(posIndex);
			return normalElement->GetDirectArray().GetAt(i);
		}
		else if (normalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
			if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				return normalElement->GetDirectArray().GetAt(polyIndex);
			int i = normalElement->GetIndexArray().GetAt(polyIndex);
			return normalElement->GetDirectArray().GetAt(i);
		}
	}
	return FbxVector4();
}

FbxVector4 GetFBXTangent(FbxGeometryElementTangent* tangentElement, int polyIndex, int posIndex) {
	if (tangentElement) {
		if (tangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
			if (tangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				return tangentElement->GetDirectArray().GetAt(posIndex);
			int i = tangentElement->GetIndexArray().GetAt(posIndex);
			return tangentElement->GetDirectArray().GetAt(i);
		}
		else if (tangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
			if (tangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				return tangentElement->GetDirectArray().GetAt(polyIndex);
			int i = tangentElement->GetIndexArray().GetAt(polyIndex);
			return tangentElement->GetDirectArray().GetAt(i);
		}
	}
	return FbxVector4();
}

FbxVector2 GetFBXTexCoord(FbxGeometryElementUV* uvElement, int polyIndex, int posIndex) {
	if (uvElement) {
		if (uvElement->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
			if (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				return uvElement->GetDirectArray().GetAt(posIndex);
			int i = uvElement->GetIndexArray().GetAt(posIndex);
			return uvElement->GetDirectArray().GetAt(i);
		}
		else if (uvElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
			if (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				return uvElement->GetDirectArray().GetAt(polyIndex);
			int i = uvElement->GetIndexArray().GetAt(polyIndex);
			return uvElement->GetDirectArray().GetAt(i);
		}
	}
	return FbxVector2();
}
#endif

void Model3D::importFbxNode(void *_node) {
#ifdef ALLOW_FBX_MODELS
	FbxNode *node = (FbxNode*)_node;
	if (!node) return;
	int childs = node->GetChildCount();
	auto& verts = getDrawData()->getVertices();
	auto& indices = getDrawData()->getIndices();
	auto& objects = getDrawData()->getObjects();
	IEngine *pEngine = engine;
	int off = (int)indices.size();
	
	int currentPosPolyIndex = 0;
	for (int i = 0; i < childs; i++) {
		FbxNode *child = node->GetChild(i);
		FbxMesh *mesh = child->GetMesh();

		if (mesh) {
			//if (!mesh->IsTriangleMesh()) continue;
			Ptr<Material> grp = gcnew<Material>();
			grp->add(pEngine->getDefaultAlpha());
			grp->add(pEngine->getDefaultTexture());
			grp->add(pEngine->getDefaultDiffuse());
			grp->add(pEngine->getDefaultBump());
			grp->add(pEngine->getDefaultMisc());
			off = (int)indices.size();

			int numVerts = mesh->GetControlPointsCount();
			verts.reserve(numVerts);

			FbxVector4* vertices = NULL;
			vertices = new FbxVector4[numVerts];
			memcpy(vertices, mesh->GetControlPoints(), numVerts * sizeof(FbxVector4));
			int voff = (int)verts.size();
			cout << "Num vertices: " << numVerts << endl;
			for (int i = 0; i < numVerts; i++) {
				Vertex vtx;
				vtx.pos = float3((float)vertices[i][0], (float)vertices[i][1], (float)vertices[i][2]);
				verts.push_back(vtx);
			}
			delete[] vertices;

			const int polyCount = mesh->GetPolygonCount();
			for (int polyIndex = 0; polyIndex < polyCount; polyIndex++)
			{
				const int lVerticeCount = mesh->GetPolygonSize(polyIndex);
				for (int vtxIdx = 0; vtxIdx < lVerticeCount; vtxIdx++)
				{
					Vertex& vtx = verts[voff + mesh->GetPolygonVertex(polyIndex, vtxIdx)];
					FbxVector4 normal = GetFBXNormal(mesh->GetElementNormal(), polyIndex, vtxIdx);
					FbxVector4 tangent = GetFBXTangent(mesh->GetElementTangent(), polyIndex, vtxIdx);
					FbxVector2 uv = GetFBXTexCoord(mesh->GetElementUV(), polyIndex, vtxIdx);
					vtx.normal = float3((float)normal[0], (float)normal[1], (float)normal[2]);
					vtx.tangent = float3((float)tangent[0], (float)tangent[1], (float)tangent[2]);
					vtx.texcoord = float2((float)uv[0], (float)uv[1]);
					indices.push_back(voff + mesh->GetPolygonVertex(polyIndex, vtxIdx));
				}
			}

			objects.push_back(gcnew<IndexBuffer>((unsigned int)indices.size(), grp, float3(0.0f, 0.0f, 0.0f), "FBX"));
		}
		this->importFbxNode(child);
	}
#endif
}

void Model3D::readMaterials(IEngine *pEngine, std::string pathPrefix, const char *mtlPath, std::map<std::string, std::shared_ptr<IMaterial> >& mtls) {
	if (mtlPath) {
		std::cout << "Opening material file: " << mtlPath << std::endl;
		
		std::ifstream f(mtlPath, std::ios_base::in);
		if (f.is_open()) {
			std::string line;
			std::string mtlName = "";
			while (std::getline(f, line)) {
				std::stringstream ss;
				ss << line;
				std::string word;
				ss >> word;
				if (word == "newmtl") {
					ss >> mtlName;
					Ptr<IMaterial> grp = pEngine->getRenderer()->createMaterial(mtlName.c_str());
					grp->add(pEngine->getDefaultAlpha());
					grp->add(pEngine->getDefaultTexture());
					grp->add(pEngine->getDefaultDiffuse());
					grp->add(pEngine->getDefaultBump());
					grp->add(pEngine->getDefaultMisc());
					//grp->add(snowDiff2); grp->add(snowNorm2); grp->add(snowMisc2);
					mtls[mtlName] = grp;
				} else if (word.find("map_") == 0) {
					std::string p; ss >> p;
					if (p[0] != '/' && p[0] != '\\') p = pathPrefix + p;
					else p = p.substr(1);
					TextureSlot type = TextureSlotAmbient;
					const char *name = "Albedo";
					if (word == "map_Ka") {
						type = TextureSlotAmbient;
						name = "Albedo";
					} else if (word == "map_d") {
						type = TextureSlotAlpha;
						name = "Alpha";
					} else if (word == "map_Bump") {
						type = TextureSlotNormal;
						name = "Normal";
					} else if (word == "map_Misc" || word == "map_Spec" || word == "map_s" || word == "map_S") {
						type = TextureSlotMisc;
						name = "Misc";
					} else if (word == "map_Kd") {
						type = TextureSlotDiffuse;
						name = "Diffuse";
					} else continue;
					mtls[mtlName]->add(pEngine->loadTexture(fixPath(p).c_str(), name, true, type));
				} else if (word.find("auto_mtl") == 0) {
					std::string ddn = "ddna";
					if (word == "auto_mtl_old") {
						ddn = "ddn";
					}
					std::string p; ss >> p;
					if (p[0] != '/' && p[0] != '\\') p = pathPrefix + p;
					else p = p.substr(1);
					int off = 0;
					if (word.find("2") != std::string::npos) {
						off = TextureSlotDiffuse2;
					} else if (word.find("3") != std::string::npos) {
						off = TextureSlotDiffuse3;
					}
					mtls[mtlName]->add(pEngine->loadTexture(fixPath(p+".dds").c_str(), "Albedo", true, TextureSlotDiffuse + off));
					mtls[mtlName]->add(pEngine->loadTexture(fixPath(p+"_"+ddn+".dds").c_str(), "DDN", true, TextureSlotNormal + off));
					mtls[mtlName]->add(pEngine->loadTexture(fixPath(p+"_misc.dds").c_str(), "Misc", true, TextureSlotMisc + off));
				} else if (word == "Ka") {
					float4 color;
					color.w = 1.0f;
					ss >> color.x >> color.y >> color.z;
					mtls[mtlName]->ambient() = color;
					continue;
				} else if (word == "Kd") {
					float4 color;
					color.w = 1.0f;
					ss >> color.x >> color.y >> color.z;
					mtls[mtlName]->diffuse() = color;
					continue;
				} else if (word == "Ks") {
					float4 color;
					color.w = 1.0f;
					ss >> color.x >> color.y >> color.z;
					mtls[mtlName]->specular() = color;
					continue;
				} else if (word == "d") {
					ss >> mtls[mtlName]->alpha();
					continue;
				} else if (word == "Kp") {
					float4 color;
					ss >> color.x >> color.y >> color.z >> color.w;
					mtls[mtlName]->pbr1() = color;
					continue;
				} else if (word == "Kpx") {
					float4 color;
					ss >> color.x >> color.y >> color.z >> color.w;
					mtls[mtlName]->pbr2() = color;
					continue;
				} else if (word == "Scale" || word == "scale") {
					float4 color;
					ss >> color.x >> color.y;
					mtls[mtlName]->scale() = color;
					continue;
				} else if (word == "Refract" || word == "refract") {
					int col; ss >> col;
					mtls[mtlName]->isRefractive() = col != 0;
				}
			}
			f.close();
		}
	}
}

void Model3D::exportModel(std::string path, std::string pathPrefix) {
	auto drawData = getDrawData();
	auto& vertices = drawData->getVertices();
	auto& indices = drawData->getIndices();
	auto& objects = drawData->getObjects();
	FILE *f = fopen(path.c_str(), "wb");
	unsigned long long indices_size = indices.size();
	int ver = FMT_VERSION;
	int signature = 0x5A5A5A5A;
	fwrite(&signature, sizeof(signature), 1, f);
	fwrite(&ver, sizeof(ver), 1, f);
	fwrite(&indices_size, sizeof(indices_size), 1, f);
	fwrite(indices.data(), sizeof(Index_t), indices_size, f);
	unsigned long long vertices_size = vertices.size();
	fwrite(&vertices_size, sizeof(vertices_size), 1, f);
	fwrite(vertices.data(), sizeof(Vertex), vertices_size, f);
	unsigned long long objects_size = objects.size();
	fwrite(&objects_size, sizeof(objects_size), 1, f);
	for (size_t i = 0; i < objects_size; i++) {
		std::string name = objects[i]->name;
		int offset = objects[i]->offset;
		fwrite(&offset, sizeof(offset), 1, f);
		auto& mtl = objects[i]->pTextures;
		unsigned char tex = (unsigned char)(mtl->size() & 0xFF);
		
		int extraLength = 0;
		void *extraSerialized = 0;
		if (objects[i]->extra) {
			// TODO serialize
		}
		
		fwrite(&extraLength, sizeof(int), 1, f);
		if (extraLength) {
			fwrite(extraSerialized, extraLength, 1, f);
		}
		
		fputc(((int)name.length() + 1)&0xFF, f);
		fputs(name.c_str(), f);
		fputc(0, f);
		
		fputc(tex, f);
		
		fputc((int)strlen(mtl->getName()) + 1, f);
		fputs(mtl->getName(), f);
		fputc(0, f);
		
		fwrite(&mtl->diffuse(), sizeof(mtl->diffuse()), 1, f);
		fwrite(&mtl->ambient(), sizeof(mtl->ambient()), 1, f);
		fwrite(&mtl->specular(), sizeof(mtl->specular()), 1, f);
		fwrite(&mtl->pbr1(), sizeof(mtl->pbr1()), 1, f);
		fwrite(&mtl->pbr2(), sizeof(mtl->pbr2()), 1, f);
		fwrite(&mtl->alpha(), sizeof(mtl->alpha()), 1, f);
		fwrite(&mtl->isRefractive(), sizeof(mtl->isRefractive()), 1, f);
		fwrite(&mtl->scale(), sizeof(mtl->scale()), 1, f);
		
		for (auto it = mtl->begin(); it != mtl->end(); it++) {
			fputc((*it).second->getSlot(), f);
			fputc((int)(*it).second->getPath().length() + 1, f);
			fputs((*it).second->getPath().c_str(), f);
			fputc(0, f);
		}
	}
	fclose(f);
}

void Model3D::importModel(std::string path, std::string pathPrefix) {
	auto pDD = getDrawData();
	auto& vertices = pDD->getVertices();
	auto& indices = pDD->getIndices();
	auto& objects = pDD->getObjects();
	if (!vertices.size() && !indices.size()) {
		FILE *f = fopen(path.c_str(), "rb");
		if (!f) return;
		unsigned long long indices_size, vertices_size, objects_size;
		int signature = 0, ver = 0;
		fread(&signature, sizeof(signature), 1, f);
		if (signature == 0x5A5A5A5A) {
			fread(&ver, sizeof(ver), 1, f);
		}
		else rewind(f);
		fread(&indices_size, sizeof(indices_size), 1, f);
		indices.resize(indices_size);
		fread(indices.data(), sizeof(Index_t), indices_size, f);
		fread(&vertices_size, sizeof(vertices_size), 1, f);
		vertices.resize(vertices_size);
		fread(vertices.data(), sizeof(Vertex), vertices_size, f);
		fread(&objects_size, sizeof(objects_size), 1, f);
		std::map<std::string, Ptr<IMaterial>> materials;
		unsigned int start = 0;
		for (size_t i = 0; i < objects_size; i++) {
			unsigned int offset = 0;
			fread(&offset, sizeof(offset), 1, f);
			char txGroupName[255];
			char objName[255];

			int extraLength = 0;
			fread(&extraLength, sizeof(int), 1, f);
			if (extraLength) {
				char *extraData = new char[extraLength];
				fread(extraData, extraLength, 1, f);
				// TODO deserialize
				delete[] extraData;
			}

			int len = fgetc(f);
			fread(objName, len, 1, f);

			unsigned char tex = fgetc(f);
			len = fgetc(f);
			fread(txGroupName, len, 1, f);
			std::string mtlName = txGroupName;
			Ptr<IMaterial> txGroup;
			bool add = false;
			if (materials.find(mtlName) == materials.end()) {
				add = true;
				txGroup = materials[mtlName] = engine->getRenderer()->createMaterial("ImportedMaterial");
			}
			else txGroup = materials[mtlName];
			unsigned int rgba;
			float r, g, b, a;

			if (ver >= 2) {
				fread(&txGroup->diffuse(), sizeof(txGroup->diffuse()), 1, f);
				fread(&txGroup->ambient(), sizeof(txGroup->ambient()), 1, f);
				fread(&txGroup->specular(), sizeof(txGroup->specular()), 1, f);
				fread(&txGroup->pbr1(), sizeof(txGroup->pbr1()), 1, f);
				fread(&txGroup->pbr2(), sizeof(txGroup->pbr2()), 1, f);
				fread(&txGroup->alpha(), sizeof(txGroup->alpha()), 1, f);
				fread(&txGroup->isRefractive(), sizeof(txGroup->isRefractive()), 1, f);
				fread(&txGroup->scale(), sizeof(txGroup->scale()), 1, f);
			}
			else {
				fread(&rgba, sizeof(rgba), 1, f);
				r = ((rgba >> 24) & 255) / 255.0f;
				g = ((rgba >> 16) & 255) / 255.0f;
				b = ((rgba >> 8) & 255) / 255.0f;
				a = ((rgba) & 255) / 255.0f;
				txGroup->diffuse() = float4(r, g, b, a);

				fread(&rgba, sizeof(rgba), 1, f);
				r = ((rgba >> 24) & 255) / 255.0f;
				g = ((rgba >> 16) & 255) / 255.0f;
				b = ((rgba >> 8) & 255) / 255.0f;
				a = ((rgba) & 255) / 255.0f;
				txGroup->ambient() = float4(r, g, b, a);

				fread(&rgba, sizeof(rgba), 1, f);
				r = ((rgba >> 24) & 255) / 255.0f;
				g = ((rgba >> 16) & 255) / 255.0f;
				b = ((rgba >> 8) & 255) / 255.0f;
				a = ((rgba) & 255) / 255.0f;
				txGroup->specular() = float4(r, g, b, a);

				if (ver >= 1) {
					fread(&rgba, sizeof(rgba), 1, f);
					r = ((rgba >> 24) & 255) / 255.0f;
					g = ((rgba >> 16) & 255) / 255.0f;
					b = ((rgba >> 8) & 255) / 255.0f;
					a = ((rgba) & 255) / 255.0f;
					txGroup->pbr1() = float4(r, g, b, a);
				}

				unsigned short alpha = 0;
				fread(&alpha, sizeof(alpha), 1, f);
				txGroup->alpha() = alpha / 65535.0f;
			}
			for (unsigned char j = 0; j < tex; j++) {
				int slot = fgetc(f);
				len = fgetc(f);
				char txPath[255];
				fread(txPath, len, 1, f);
				if (add) {
					txGroup->add(engine->loadTexture(txPath, "Imported", true, slot));
				}
			}
			objects.push_back(gcnew<IndexBuffer>(offset, txGroup, float3(0.0f, 0.0f, 0.0f), objName));
		}
		fclose(f);
		pDD->finish();
	}
}

void Model3D::postInit() {
	auto pDD = getDrawData();
	unsigned int lastOffset = 0;
	std::vector<Ptr<IndexBuffer>> newObjects;
	auto& vertices = pDD->getVertices();
	auto& indices = pDD->getIndices();
	auto& objects = pDD->getObjects();
	unsigned long ctr = 0;

	int corrupted = 0;
	for (auto& ind : indices) {
		if (ind < 0 || ind >= vertices.size()) {
			ind %= vertices.size();
			corrupted++;
		}
	}

	for (auto& it : objects) {
		unsigned int off = it->offset;
		unsigned int length = off - lastOffset;
		if (length > 0) {
			float x = 0.0f, y = 0.0f, z = 0.0f;
			int l = 0;
			
			it->start = lastOffset;
			
			for (unsigned int i = lastOffset; i < off; i++) {
				unsigned int idx = indices[i];
				if (idx >= vertices.size()) continue;
				Vertex vtx = vertices[idx];
				x += vtx.pos.x;
				y += vtx.pos.y;
				z += vtx.pos.z;
				l++;
			}
			
			x /= l;
			y /= l;
			z /= l;
			
			float mxdst = 0.0f;
			float mxx = 0.0f, mxy = 0.0f, mxz = 0.0f;
			float3 furtherMost, furtherMostX, furtherMostY, furtherMostZ;
			for (unsigned int i = lastOffset; i < off; i++) {
				unsigned int idx = indices[i];
				if (idx >= vertices.size()) continue;
				Vertex vtx = vertices[idx];
				float dx = vtx.pos.x - x, dy = vtx.pos.y - y, dz = vtx.pos.z - z;
				if (dx < 0) dx = -dx;
				if (dy < 0) dy = -dy;
				if (dz < 0) dz = -dz;
				float sqdst = dx * dx + dy * dy + dz * dz;
				if (sqdst > mxdst) {
					mxdst = sqdst;
					furtherMost = vtx.pos;
				}
				if (dx > mxx) {
					mxx = dx; furtherMostX = vtx.pos;
				}
				if (dy > mxy) {
					mxy = dy; furtherMostY = vtx.pos;
				}
				if (dz > mxz) {
					mxz = dz; furtherMostZ = vtx.pos;
				}
			}
			
			/*
			printf("further most: %f %f %f for %s\n", furtherMost.x, furtherMost.y, furtherMost.z, it.name.c_str());
			printf("further most x: %f %f %f for %s\n", furtherMostX.x, furtherMostX.y, furtherMostX.z, it.name.c_str());
			printf("further most y: %f %f %f for %s\n", furtherMostY.x, furtherMostY.y, furtherMostY.z, it.name.c_str());
			printf("further most z: %f %f %f for %s\n", furtherMostZ.x, furtherMostZ.y, furtherMostZ.z, it.name.c_str());
			*/
			it->relativePos = float3(x, y, z);
			it->radius = sqrtf(mxdst);
			it->axisRadius = float3(mxx, mxy, mxz);
			it->id = ctr++;
			newObjects.push_back(it);
		}
		lastOffset = off;
	}
	pDD->getObjects() = newObjects;
}

Model3D::Model3D(std::string szPath) : path(szPath), auto_rotate(false) {}

IObject3D& Model3D::create(void *params) {
	ModelSpawnParams *spawnParams = (ModelSpawnParams*)params;
	if (params) {
		path = spawnParams->path;
	}
	return *this;
}

Model3D::Model3D() : path(""), auto_rotate(false) {}

void Model3D::update(const IWorld *pWorld) {
	Object3D::update(pWorld);
	if (animated){
		Animated::update(pWorld);
	}
}