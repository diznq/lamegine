#pragma once
#include "Interfaces.h"


class DrawDataManager;
class DrawData : public IDrawData {
private:
	friend class Engine;
	friend class DrawDataManager;
	IDrawDataManager* parent;
	std::vector<Vertex> vertices;
	std::vector<Index_t> indices;
	std::vector<Ptr<IndexBuffer>> objects;
	std::deque<IObject3D*> refObjects;
	std::string path;
	unsigned int bufferId;
	unsigned int totalIndices;
	bool cleanUp;
	Ptr<IBuffer> pVertexBuffer;
	Ptr<IBuffer> pIndexBuffer;
	bool bound;
	int refs;
	unsigned long long lastFrame;
	int lastPass;
public:
	DrawData();
	virtual ~DrawData();
	virtual bool& isBound();
	virtual bool& needsCleanUp();
	virtual Ptr<IBuffer> getIndexBuffer();
	virtual Ptr<IBuffer> getVertexBuffer();
	virtual void setIndexBuffer(Ptr<IBuffer> buffer);
	virtual void setVertexBuffer(Ptr<IBuffer> buffer);
	virtual void addRef(IObject3D* pObj);
	virtual void release(IObject3D* pObj);
	virtual void finish();
	virtual std::vector<Vertex>& getVertices();
	virtual std::vector<Index_t>& getIndices();
	virtual std::vector<Ptr<IndexBuffer>>& getObjects();
	virtual unsigned int getTotalIndices() const;
	virtual unsigned int getBufferId() const;
	virtual void setBufferId(unsigned int id);
};

class DrawDataManager : public IDrawDataManager {
private:
	std::map<std::string, std::weak_ptr<IDrawData> > data;
	unsigned int counter;
public:
	DrawDataManager();
	virtual ~DrawDataManager();
	virtual bool remove(std::string path);
	virtual Ptr<IDrawData> getDrawData(std::string path, bool cache = true);
};
