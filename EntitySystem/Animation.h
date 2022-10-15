#pragma once
#include "../Lamegine/Interfaces.h"

class Joint : public IJoint {
protected:
	matrix transform;
	matrix localBindTransform;
	matrix inverseTransform;
	int id;
	std::string name;
	std::vector<Ptr<IJoint>> children;
	float3 position;
	float4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
public:
	Joint() {}
	Joint(int _id, std::string n, matrix transf) : id(_id), name(n), transform(transf), localBindTransform(transf) {}
	virtual void reset(int _id, const std::string& n, matrix mtx);
	virtual void addChild(Ptr<IJoint> joint);
	virtual matrix& getTransform();
	virtual void setTransform(matrix& mtx);
	virtual std::string& getName();
	virtual matrix& getBindTransform();
	virtual matrix& getInverseBindTransform();
	virtual void calcInverseBindTransform(const matrix& parentBindTransform);
	virtual std::vector<Ptr<IJoint>>& getChildren();
	virtual int getId() const;
	virtual void setPosition(float3 pos);
	virtual float3 getPosition() const;
	virtual void setRotation(float4 pos);
	virtual float4 getRotation() const;
	virtual void printHierarchy(Ptr<IJoint> parent, int offset) const;
	virtual Ptr<IJoint> getJointByName(const std::string& name);
};

struct KeyFrame : public IKeyFrame {
protected:
	std::unordered_map<std::string, JointTransform> jointTransforms;
	float timeStamp;
public:
	KeyFrame() {}
	KeyFrame(float ts, const std::unordered_map<std::string, JointTransform>& jts) : timeStamp(ts), jointTransforms(jts) {}
	virtual std::unordered_map<std::string, JointTransform>& getJointTransforms(); 
	virtual float getTimestamp() const;
};

class Animation : public IAnimation {
protected:
	std::vector<Ptr<IKeyFrame>> keyFrames;
	float length;
public:
	Animation(float len, const std::vector<Ptr<IKeyFrame>>& kfs) : length(len), keyFrames(kfs) {}
	virtual std::vector<Ptr<IKeyFrame>>& getKeyFrames();
	virtual float getLength() const;
};

class Animator : public IAnimator {
protected:
	Ptr<IAnimation> animation;
	Ptr<IJoint> rootJoint;
	float animationTime;
	std::unordered_map<std::string, JointTransform> calculateCurrentAnimationPose();
	void getPrevNextFrames(Ptr<IKeyFrame> out[2]);
	float calculateProgression(Ptr<IKeyFrame> previousFrame, Ptr<IKeyFrame> nextFrame);
	std::unordered_map<std::string, JointTransform> interpolatePoses(Ptr<IKeyFrame> previousFrame, Ptr<IKeyFrame> nextFrame, float progression);
	void applyPoseToJoints(std::unordered_map<std::string, JointTransform>& currentPose, Ptr<IJoint> joint, matrix parentTransform);
public:
	Animator(Ptr<IJoint> _rootJoint) : rootJoint(_rootJoint) {}
	virtual void startAnimation(Ptr<IAnimation> anim);
	virtual void update(float dt);
};

class Animated : public IAnimated {
protected:
	Ptr<IJoint> rootJoint;
	Ptr<IAnimator> animator;
	std::vector<matrix> jointTransforms;
	std::unordered_map<std::string, int> jointMapping;
	std::unordered_map<std::string, Ptr<IAnimation>> animations;
	int jointCount = 0;
	void addJointsToArray(Ptr<IJoint> headJoint, std::vector<matrix>& jointMatrices);
public:
	virtual int getJointCount() const;
	virtual Ptr<IJoint> getRootJoint();
	virtual Ptr<IAnimator> getAnimator();
	virtual std::vector<matrix>& getJointTransforms();
	virtual void startAnimation(Ptr<IAnimation> animation);
	virtual void update(const IWorld *pWorld);
	virtual Ptr<IAnimation> getAnimation(const std::string& name);
	virtual std::unordered_map<std::string, Ptr<IAnimation>>& getAnimations();
	virtual Ptr<IJoint> getJointByName(const std::string& name);
};