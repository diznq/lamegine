#pragma once
#include "VectorMath.h"
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>
#include <math.h>
namespace Math {
	static Vec3 vectorFloor(Vec3 v, float pwr) {
		return Vec3(
			::floor(v.x*pwr) / pwr,
			::floor(v.y*pwr) / pwr,
			::floor(v.z*pwr) / pwr);
	}
	static Vec3 toVec3(btVector3& btVec) {
		#ifdef _MSC_VER
		return Vec3((XMVECTOR)btVec.get128());
		#else
		return Vec3(btVec.getX(), btVec.getY(), btVec.getZ());
		#endif
	}
	static Vec4 toVec4(btQuaternion btQuat) {
		#ifdef _MSC_VER
		return Vec4((XMVECTOR)btQuat.get128());
		#else
		return Vec4(btQuat.getX(), btQuat.getY(), btQuat.getZ(), btQuat.getW());
		#endif
	}
}