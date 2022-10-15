#pragma once
#include <mmintrin.h>
#include <DirectXMath.h>
#include <iostream>
#include <stdio.h>

using namespace DirectX;

namespace Math {

	class Mat4 : public XMFLOAT4X4 {
	public:
		using XMFLOAT4X4::operator();
		using XMFLOAT4X4::operator=;
		using XMFLOAT4X4::XMFLOAT4X4;

		Mat4() : XMFLOAT4X4() {

		}
		Mat4(float scale) : XMFLOAT4X4()
		{
			XMStoreFloat4x4(this, XMMatrixScaling(scale, scale, scale));
		}
		Mat4(const XMMATRIX mtx) {
			XMStoreFloat4x4(this, mtx);
		}
		Mat4 transpose() const {
			return Mat4(XMMatrixTranspose(XMLoadFloat4x4(this)));
		}
		Mat4 inverse() const {
			return Mat4(XMMatrixInverse(nullptr, XMLoadFloat4x4(this)));
		}
		Mat4 operator*(const Mat4& b) const {
			return Mat4(XMMatrixMultiply(XMLoadFloat4x4(this), XMLoadFloat4x4(&b)));
		}
		Mat4 operator*(float f) const {
			return Mat4(XMMatrixMultiply(XMLoadFloat4x4(this), Mat4(f).raw()));
		}
		void print() {
			printf("[ %5.4f, %5.4f, %5.4f, %5.4f,\n"
				"  %5.4f, %5.4f, %5.4f, %5.4f,\n"
				"  %5.4f, %5.4f, %5.4f, %5.4f,\n"
				"  %5.4f, %5.4f, %5.4f, %5.4f ]\n", _11, _12, _13, _14, _21, _22, _23, _24, _31, _32, _33, _34, _41, _42, _43, _44);
		}
		XMMATRIX raw() const { return XMLoadFloat4x4(this); }
	};

	template<class T,
		XMVECTOR __vectorcall LoadFn(const T*),
		void __vectorcall StoreFn(T*, XMVECTOR)>
		class GenericVector : public T {
		public:
			using T::T;

			inline XMVECTOR raw() const {
				return LoadFn(this);
			}
			inline XMVECTOR load(const void* data) const {
				return LoadFn((const T*)data);
			}
			inline void store(void* data, XMVECTOR xm) {
				StoreFn((T*)data, xm);
			}

			GenericVector() {
				for (size_t i = 0, j = sizeof(GenericVector) / sizeof(float); i < j; i++) {
					((float*)this)[i] = 0.0f;
				}
			}
			GenericVector(float f) {
				for (size_t i = 0, j = sizeof(GenericVector) / sizeof(float); i < j; i++) {
					((float*)this)[i] = f;
				}
			}
			GenericVector(XMVECTOR xm) {
				StoreFn(this, xm);
			}

			GenericVector(const GenericVector& v) {
				StoreFn(this, v.raw());
				//assert(dot(this) == v.dot(v));
			}
			template<typename T1 = GenericVector >
			GenericVector(T1 v, float b = 1.0f, float c = 1.0f, float d = 1.0f) {
				XMFLOAT4 src(0.0f, b, c, d);
				StoreFn(this, XMLoadFloat4(&src));
				XMVECTOR vec = v.load(&v);
				v.store(this, vec);
			}

			GenericVector& operator=(const GenericVector& v) {
				StoreFn(this, v.raw());
				//assert(dot(this) == v.dot(v));
				return *this;
			}

			inline GenericVector operator*(float f) {
				GenericVector n(*this);
				for (size_t i = 0, j = sizeof(GenericVector) / sizeof(float); i < j; i++) {
					((float*)&n)[i] *= f;
				}
				return n;
			}
			inline GenericVector operator/(float f) {
				GenericVector n(*this);
				for (size_t i = 0, j = sizeof(GenericVector) / sizeof(float); i < j; i++) {
					((float*)&n)[i] /= f;
				}
				return n;
			}
			inline GenericVector& operator*=(float f) {
				for (size_t i = 0, j = sizeof(GenericVector) / sizeof(float); i < j; i++) {
					((float*)this)[i] *= f;
				}
				return *this;
			}
			inline GenericVector& operator/=(float f) {
				for (size_t i = 0, j = sizeof(GenericVector) / sizeof(float); i < j; i++) {
					((float*)this)[i] /= f;
				}
				return *this;
			}

			inline GenericVector operator+(XMVECTOR vec) {
				return GenericVector(XMVectorAdd(LoadFn(this), vec));
			}
			inline GenericVector operator-(XMVECTOR vec) {
				return GenericVector(XMVectorSubtract(LoadFn(this), vec));
			}
			inline GenericVector operator*(XMVECTOR vec) {
				return GenericVector(XMVectorMultiply(LoadFn(this), vec));
			}
			inline float dot(XMVECTOR vec) {
				return GenericVector(XMVector4Dot(LoadFn(this), vec)).x;
			}
			inline GenericVector cross(XMVECTOR vec) {
				return GenericVector(XMVector3Cross(LoadFn(this), vec));
			}

			inline GenericVector& operator+=(XMVECTOR vec) {
				StoreFn(this, XMVectorAdd(LoadFn(this), vec));
				return *this;
			}
			inline GenericVector& operator-=(XMVECTOR vec) {
				StoreFn(this, XMVectorSubtract(LoadFn(this), vec));
				return *this;
			}

			inline GenericVector operator*(const Mat4& m) {
				return GenericVector(XMVector4Transform(LoadFn(this), XMLoadFloat4x4(&m)));
			}

			inline operator XMVECTOR() const {
				return LoadFn(this);
			}

			inline float length() { return sqrt((*this).dot(*this)); }
			inline float dist(GenericVector b) const { GenericVector tmp = b - *this; return sqrt(tmp.dot(tmp)); }
			inline GenericVector normalize() const {
				return GenericVector(XMVector4Normalize(LoadFn(this)));
			}

			friend std::ostream & operator<<(std::ostream &os, const GenericVector& p) {
				size_t sz = sizeof(p) / sizeof(float);
				const float *start = (const float*)&p;
				for (size_t i = 0; i < sz; i++) {
					os << start[i];
					if (i != (sz - 1)) os << ", ";
				}
				return os;
			}
	};

	typedef GenericVector<DirectX::XMFLOAT4, DirectX::XMLoadFloat4, DirectX::XMStoreFloat4> Vec4;
	typedef GenericVector<DirectX::XMFLOAT3, DirectX::XMLoadFloat3, DirectX::XMStoreFloat3> Vec3;
	typedef GenericVector<DirectX::XMFLOAT2, DirectX::XMLoadFloat2, DirectX::XMStoreFloat2> Vec2;

	template<class T> static T round(T n) {
		::round(n);
	}

	static float radians(float n) {
		return XM_PI * n / 180.0f;
	}

	static Mat4 lookAt(Vec3 eye, Vec3 dir, Vec3 up) {
		return Mat4(XMMatrixTranspose(XMMatrixLookAtRH(eye.raw(), dir.raw(), up.raw())));
	}
	static Mat4 scale(Mat4 mtx, Vec3 vec) {
		return mtx;
	}
	static Mat4 perspective(float yangl, float ratio, float nearz, float farz) {
		return Mat4(XMMatrixTranspose(XMMatrixPerspectiveFovRH(yangl, ratio, nearz, farz)));
	}
	static Mat4 ortho(float a, float b, float c, float d, float e, float f) {
		return Mat4(XMMatrixTranspose(XMMatrixOrthographicOffCenterRH(a, b, c, d, e, f)));
	}
}


static Math::Vec3 TransformFloat3(const Math::Vec3& point, const Math::Mat4& matrix) {
	Math::Vec3 result;
	XMFLOAT4 temp(point.x, point.y, point.z, 1);
	XMFLOAT4 temp2;

	temp2.x = temp.x * matrix._11 + temp.y * matrix._21 + temp.z * matrix._31 + temp.w * matrix._41;
	temp2.y = temp.x * matrix._12 + temp.y * matrix._22 + temp.z * matrix._32 + temp.w * matrix._42;
	temp2.z = temp.x * matrix._13 + temp.y * matrix._23 + temp.z * matrix._33 + temp.w * matrix._43;
	temp2.w = temp.x * matrix._14 + temp.y * matrix._24 + temp.z * matrix._34 + temp.w * matrix._44;

	result.x = temp2.x / temp2.w;
	result.y = temp2.y / temp2.w;
	result.z = temp2.z / temp2.w;

	return result;
}
static Math::Vec3 TransformTransposedFloat3(const Math::Vec3& point, const Math::Mat4& matrix) {
	Math::Vec3 result;
	XMFLOAT4 temp(point.x, point.y, point.z, 1);
	XMFLOAT4 temp2;

	temp2.x = temp.x * matrix._11 + temp.y * matrix._12 + temp.z * matrix._13 + temp.w * matrix._14;
	temp2.y = temp.x * matrix._21 + temp.y * matrix._22 + temp.z * matrix._23 + temp.w * matrix._24;
	temp2.z = temp.x * matrix._31 + temp.y * matrix._32 + temp.z * matrix._33 + temp.w * matrix._34;
	temp2.w = temp.x * matrix._41 + temp.y * matrix._42 + temp.z * matrix._43 + temp.w * matrix._44;

	result.x = temp2.x / temp2.w;
	result.y = temp2.y / temp2.w;
	result.z = temp2.z / temp2.w;

	return result;
}

#define mul TransformTransposedFloat3

#ifndef PI
#define PI XM_PI
#endif

#ifndef DEG
#define DEG (180.0f/PI)
#endif