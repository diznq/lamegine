#include "shared.h"
#include "DrawData.h"

DrawData::DrawData() : cleanUp(false), refs(0), bufferId(0), bound(false), lastFrame(0), lastPass(0), totalIndices(0) {
	//...
}
DrawData::~DrawData() {
	printf(" [gc] Released draw data of %s\n", path.c_str());
}
bool& DrawData::isBound() { return bound; }
bool& DrawData::needsCleanUp() { return cleanUp; }
Ptr<IBuffer> DrawData::getIndexBuffer() { return pIndexBuffer; }
Ptr<IBuffer> DrawData::getVertexBuffer() { return pVertexBuffer; }
void DrawData::setIndexBuffer(Ptr<IBuffer> buffer) { pIndexBuffer = buffer; }
void DrawData::setVertexBuffer(Ptr<IBuffer> buffer) { pVertexBuffer = buffer; }
void DrawData::addRef(IObject3D* pObj) { refObjects.push_back(pObj); refs++; }
void DrawData::finish() {
	totalIndices = (unsigned int)indices.size();
}
std::vector<Vertex>& DrawData::getVertices() {
	return vertices;
}
std::vector<Index_t>& DrawData::getIndices() {
	return indices;
}
std::vector<Ptr<IndexBuffer>>& DrawData::getObjects() {
	return objects;
}
unsigned int DrawData::getTotalIndices() const {
	return totalIndices;
}
unsigned int DrawData::getBufferId() const { return bufferId; }
void DrawData::setBufferId(unsigned int id) { bufferId = id; }

void DrawData::release(IObject3D* pObj) {
	std::deque<IObject3D*>::iterator it;
	bool found = false;
	for (it = refObjects.begin(); it != refObjects.end(); it++) {
		if (*it == pObj) {
			found = true;
			break;
		}
	}
	if (found) {
		refObjects.erase(it);
	}
	refs--;
	if (refs == 0) {
		if (parent) {
			parent->remove(path);
		}
		vertices.clear();
		indices.clear();
	}

}
bool DrawDataManager::remove(std::string path) {
	auto it = data.find(path);
	if (it != data.end()) {
		data.erase(it);
		return true;
	}
	return false;
}
DrawDataManager::DrawDataManager() {
	counter = 0;
}
DrawDataManager::~DrawDataManager() {

}
Ptr<IDrawData> DrawDataManager::getDrawData(std::string path, bool cache) {
	auto it = data.find(path);
	bool alloc = false;
	if (it != data.end() && (!it->second.lock())) {
		alloc = true;
		data.erase(it);
	}
	if (it == data.end() || alloc) {
		Ptr<DrawData> drawData = gcnew<DrawData>();
		drawData->bufferId = counter++;
		drawData->bound = false;
		drawData->path = path;
		drawData->parent = this;
		if (cache) data[path] = drawData;
		return drawData;
	}
	return it->second.lock();
}