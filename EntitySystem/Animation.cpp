#include "Animation.h"

void Joint::addChild(Ptr<IJoint> joint) {
	children.push_back(joint);
}

matrix& Joint::getTransform() {
	return transform;
}

matrix& Joint::getBindTransform() {
	return localBindTransform;
}

matrix& Joint::getInverseBindTransform() {
	return inverseTransform;
}

void Joint::setRotation(float4 rot) {
	rotation = rot;
}

float4 Joint::getRotation() const {
	return rotation;
}

void Joint::reset(int _id, const std::string& n, matrix mtx) {
	id = _id; name = n; localBindTransform = mtx; inverseTransform = mtx.inverse();
}

void Joint::calcInverseBindTransform(const matrix& parentBindTransform) {
	matrix bindTransform = parentBindTransform * localBindTransform;
	//inverseTransform = bindTransform.inverse();
	for (auto child : children) {
		child->calcInverseBindTransform(bindTransform);
	}
}

std::vector<Ptr<IJoint>>& Joint::getChildren() {
	return children;
}

int Joint::getId() const {
	return id;
}

std::string& Joint::getName() {
	return name;
}

void Joint::setTransform(matrix& mtx) {
	transform = mtx;
}

void Joint::setPosition(float3 pos) {
	this->position = pos;
}

float3 Joint::getPosition() const {
	return position;
}

void Joint::printHierarchy(Ptr<IJoint> parent, int offset) const {
	for (int i = 0; i < offset; i++) {
		std::cout << "  ";
	}
	for (auto it : children) {
		it->printHierarchy(nullptr, offset + 1);
	}
}

std::unordered_map<std::string, JointTransform>& KeyFrame::getJointTransforms() {
	return jointTransforms;
}

float KeyFrame::getTimestamp() const {
	return timeStamp;
}

std::vector<Ptr<IKeyFrame>>& Animation::getKeyFrames() {
	return keyFrames;
}

float Animation::getLength() const {
	return length;
}

void Animator::startAnimation(Ptr<IAnimation> anim) {
	animation = anim;
	animationTime = 0.0f;
}

void Animator::update(float dt) {
	if (!animation) return;
	animationTime += dt;
	const float len = animation->getLength();
	while (animationTime > len) animationTime -= len;
	auto currentPose = calculateCurrentAnimationPose();
	applyPoseToJoints(currentPose, rootJoint, matrix(XMMatrixIdentity()));
}

void Animator::applyPoseToJoints(std::unordered_map<std::string, JointTransform>& currentPose, Ptr<IJoint> joint, matrix parentTransform) {
	auto it = currentPose.find(joint->getName());
	matrix currentLocalTransform(1.0f);
	if (it == currentPose.end()) {
		//...
	} else {
		it->second.rotation = XMQuaternionMultiply(it->second.rotation.raw(), joint->getRotation().raw());
		currentLocalTransform = it->second.getLocalTransform();
	}
	matrix currentTransform = parentTransform * currentLocalTransform;
	for (auto childJoint : joint->getChildren()) {
		applyPoseToJoints(currentPose, childJoint, currentTransform);
	}
	currentTransform = currentTransform * joint->getInverseBindTransform();
	joint->setTransform(currentTransform);
}

std::unordered_map<std::string, JointTransform> Animator::calculateCurrentAnimationPose() {
	Ptr<IKeyFrame> frames[2];
	getPrevNextFrames(frames);
	float progression = calculateProgression(frames[0], frames[1]);
	return interpolatePoses(frames[0], frames[1], progression);
}

std::unordered_map<std::string, JointTransform> Animator::interpolatePoses(Ptr<IKeyFrame> previousFrame, Ptr<IKeyFrame> nextFrame, float progression) {
	std::unordered_map<std::string, JointTransform> currentPose;
	for (auto& it : previousFrame->getJointTransforms()) {
		JointTransform previousTransform = it.second;
		JointTransform nextTransform = nextFrame->getJointTransforms()[it.first];
		JointTransform currentTransform = JointTransform::interpolate(previousTransform, nextTransform, progression);
		currentPose[it.first] = currentTransform;
	}
	return currentPose;
}

void Animator::getPrevNextFrames(Ptr<IKeyFrame> frames[2]) {
	auto& allFrames = animation->getKeyFrames(); 
	auto previousFrame = allFrames[0];
	auto nextFrame = allFrames[0];
	for (int i = 1, j = (int)allFrames.size(); i < j; i++) {
		nextFrame = allFrames[i];
		if (nextFrame->getTimestamp() > animationTime) {
			break;
		}
		previousFrame = allFrames[i];
	}
	frames[0] = previousFrame;
	frames[1] = nextFrame;
}

float Animator::calculateProgression(Ptr<IKeyFrame> previousFrame, Ptr<IKeyFrame> nextFrame) {
	float totalTime = nextFrame->getTimestamp() - previousFrame->getTimestamp();
	float currentTime = animationTime - previousFrame->getTimestamp();
	//cout << "Progress: " << (currentTime / totalTime) << endl;
	return currentTime / totalTime;
}

void Animated::update(const IWorld *pWorld) {
	if(animator) animator->update((float)pWorld->delta);
}

Ptr<IJoint> Animated::getRootJoint() {
	return rootJoint;
}

Ptr<IAnimator> Animated::getAnimator() {
	return animator;
}

std::vector<matrix>& Animated::getJointTransforms() {
	if (!jointTransforms.size()) jointTransforms.resize(jointCount);
	addJointsToArray(rootJoint, jointTransforms);
	return jointTransforms;
}

void Animated::startAnimation(Ptr<IAnimation> animation) {
	animator->startAnimation(animation);
}

void Animated::addJointsToArray(Ptr<IJoint> headJoint, std::vector<matrix>& jointMatrices) {
	jointMatrices[headJoint->getId()] = headJoint->getTransform();
	for (Ptr<IJoint> childJoint : headJoint->getChildren()) {
		addJointsToArray(childJoint, jointMatrices);
	}
}

int Animated::getJointCount() const {
	return jointCount;
}

Ptr<IAnimation> Animated::getAnimation(const std::string& name) {
	auto it = animations.find(name);
	if (it == animations.end()) return nullptr;
	return it->second;
}

std::unordered_map<std::string, Ptr<IAnimation>>& Animated::getAnimations() {
	return animations;
}

Ptr<IJoint> Animated::getJointByName(const std::string& name) {
	return rootJoint->getJointByName(name);
}

Ptr<IJoint> Joint::getJointByName(const std::string& name) {
	if (name == this->name) return shared_from_this();
	for (auto child : children) {
		Ptr<IJoint> search = child->getJointByName(name);
		if(search) return search;
	}
	return nullptr;
}