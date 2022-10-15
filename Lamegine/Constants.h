#ifdef __cplusplus
#pragma once
#ifdef _MSC_VER
#define UNIFORM_PFX &
#else
#define UNIFORM_PFX
#endif
#define REF_PFX UNIFORM_PFX
#include "VectorMath.h"
#ifndef HLSL_COMPAT
#define HLSL_COMPAT
typedef Math::Vec4 float4;
typedef Math::Vec3 float3;
typedef Math::Vec2 float2;
typedef unsigned int uint;
typedef Math::Mat4 matrix;
struct uint2 {
	uint x;
	uint y;
#ifdef __cplusplus
	uint2(uint _x, uint _y) : x(_x), y(_y) {}
	uint2() : x(0), y(0) {}
	uint2(const uint2& b) : x(b.x), y(b.y) {}
	uint2& operator=(const uint2& b) { x = b.x; y = b.y; return *this; }
#endif
};
struct uint3 {
	uint x;
	uint y;
	uint z;
#ifdef __cplusplus
	uint3(uint _x, uint _y, uint _z) : x(_x), y(_y), z(_z) {}
	uint3() : x(0), y(0), z(0) {}
	uint3(const uint3& b) : x(b.x), y(b.y), z(b.z) {}
	uint3& operator=(const uint3& b) { x = b.x; y = b.y; z = b.z; return *this; }
#endif
};
struct uint4 {
	uint x;
	uint y;
	uint z;
	uint w;
#ifdef __cplusplus
	uint4(uint _x, uint _y, uint _z, uint _w) : x(_x), y(_y), z(_z), w(_w) {}
	uint4() : x(0), y(0), z(0), w(0) {}
	uint4(const uint4& b) : x(b.x), y(b.y), z(b.z), w(b.w) {}
	uint4& operator=(const uint4& b) { x = b.x; y = b.y; z = b.z; w = b.w; return *this; }
#endif
};
#define cbuffer struct
#define REG(n) /* ... */
#endif
#else
#define REG(n) : register(n)
#endif

//#define HAS_FEEDBACK_PASS

#define VERTEX_SHADER 1
#define FRAGMENT_SHADER 2
#define HULL_SHADER 3
#define DOMAIN_SHADER 4
#define GEOMETRY_SHADER 5

#define Z_NEAR 0.1f
#define Z_FAR 2048.0f
#define NUM_LIGHTS 4
#define NUM_REAL_CASCADES 3
#define NUM_CASCADES 4
#define NUM_SHADOW_CASCADES NUM_REAL_CASCADES
#define CASCADE_OFFSET 1
#define MAX_OBJ_PER_INST 24
#define MAX_INSTANCES 256

//#define POOR_REFRACTION

#define POSTFX 0x10000

#define POSTFX_LIGHTING		(POSTFX | 1)
#define POSTFX_SECONDARY	(POSTFX | 2)
#define POSTFX_BAKE			(POSTFX | 3)
#define POSTFX_OTHER		(POSTFX | 4)

#define ENT_ALL 0xFFFF
#define ENT_RENDER			1
#define ENT_LIGHT			2
#define ENT_CAST_SHADOW		4
#define ENT_TRANSPARENT		8

#define ENT_MAINPASS (ENT_ALL & (~ENT_LIGHT) & (~ENT_TRANSPARENT))
#define ENT_SHADOWPASS (ENT_CAST_SHADOW & (~ENT_LIGHT) & (~ENT_TRANSPARENT))

#define ENT_GEN_OBJECT (ENT_ALL) & (~ENT_LIGHT) & (~ENT_TRANSPARENT)

#define DEFAULT_OBJECT 0
#define TERRAIN 1
#define WATER 2
#define FRMBFR 3
#define QUAD 4
#define REFRACTIVE_QUAD 5
#define GRASS 6
#define PREGEN_TERRAIN 7

#define VEHICLE 32

#define MAIN_PASS 0
#define REFLECTION_PASS 1
#define SHADOW_PASS 2
#define REFRACTION_PASS 3
#define AO_PASS 4
#define DYN_SHADOW_PASS 5
#define DYN_REFR_PASS 6
#define SHADOW_POSTFX_PASS 7
#define SSR_PASS 8
#define DOWNSCALE_PASS 9
#define PASSES_LENGTH 10

#define DEPTH_BUFFER 0x4000
#define DOUBLE_DEPTH_BUFFER 0x8000
#define SIGNED 0x2000

#define SKY_COLOR float3(0.3f, 0.6f, 1.2f)
#define SHADOW_COLOR (SKY_COLOR / 10.0f)
#define SUN_COLOR float3(1.0f, 1.0f, 1.0f)

struct Light {
	float4 color;
	float3 position;
	float intensity;
#ifdef __cplusplus
	Light() : color(0.0f, 0.0f, 0.0f, 1.0f), position(0.0f, 0.0f, 0.0f), intensity(0.0f) {}
	Light(float4 col, float3 pos, float i) : color(col), position(pos), intensity(i) {}
#endif
};

struct Letter {
	float2 position;
	uint2 id;
	float4 color;
#ifdef __cplusplus
	Letter() : position(0.0f, 0.0f), id(0, 0), color(1.0f, 0.0f, 0.0f, 1.0f) {}
#endif
};

struct InstanceData {
	matrix model;

	float4 ambient;
	float4 diffuse;

	float4 pbrInfo;
	float4 pbrExtra;
	float2 txScale;

	uint order;
	uint nodeId;

	uint4 helper;
};

cbuffer SceneConstants REG(b0) {
	matrix world;
	matrix view;
	matrix proj;

	matrix invView;
	matrix invProj;

	matrix cameraView;
	matrix cameraProj;

	matrix shadowView[NUM_REAL_CASCADES];
	matrix shadowProj[NUM_REAL_CASCADES];

	float4 cameraPos;
	float4 cameraDir;
	float4 sunDir;
	float2 resolution;

	float time;
	uint frame;
	float gamma;
	float exposure;
	float bloomStrength;
	float shadowStrength;
	
	float4 radiosityRes;
	float4 radiositySettings;

	float4 cubemapPos;
	float4 cubemapBboxA;
	float4 cubemapBboxB;

	float4 sunColor;
	float4 skyColor;
	float4 fogSettings;

	float globalQuality;
	float realGlobalQuality;
	float shadowQuality;
	float distortionStrength;

	float4 hdrSettings;
};

cbuffer EntityConstants REG(b1) {
	InstanceData instances[MAX_INSTANCES];
};

cbuffer AnimationConstants REG(b2) {
	matrix jointTransforms[64];
};

cbuffer LightConstants REG(b2) {
	Light lights[NUM_LIGHTS];
	uint lightCount;
	uint l_reserved1;
	uint l_reserved2;
	uint l_reserved3;
};

cbuffer TextConstants REG(b2) {
	Letter letters[256];
};

cbuffer InstanceConstants REG(b3) {
	uint4 instance_start;
};

#ifndef __cplusplus
struct VS_INPUT
{
	float3 pos		: POSITION;
	float2 texcoord	: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
	uint3 jointIds  : BLENDINDICES;
	float3 weights  : BLENDWEIGHT;
	uint InstanceID : SV_InstanceID;
};
#endif