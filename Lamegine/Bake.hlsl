#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

struct PS_OUTPUT {
	float4 Color		: SV_TARGET;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D ColorTexture		: register(t0);
Texture2D ShadowTexture		: register(t1);
Texture2D SSRTexture		: register(t1);
Texture2D SSRTextureBlur	: register(t2);
Texture2D Misc				: register(t2);
Texture2D ReflMap			: register(t3);
Texture2D Geometry			: register(t4);
Texture2D Normal			: register(t5);
Texture2D Distortion		: register(t4);
Texture2D ColorDistortion	: register(t5);
Texture2D SSRPos			: register(t6);
Texture2D SpecularBRDFLUT	: register(t7);

//#define USE_FRESNEL
#include "FxLibrary.hlsli"
#define PI 3.14159265359
#include "PBR.hlsli"

float4 SampleReflection(float3 dir, float rougness)
{
	dir = normalize(dir);
	half theta = acos(dir.z); // +z is up
	half phi = atan2(-dir.y, -dir.x) + PI;
	half2 uv = half2(phi, theta) * half2(0.1591549, 0.31830988618);
	return ReflMap.SampleLevel(MainSampler, uv, rougness);
}

float3 LocalCorrect(float3 origVec, float3 bboxMin, float3 bboxMax, float3 vertexPos, float3 cubemapPos) {
	float3 invOrigVec = float3(1.0f, 1.0f, 1.0f) / (origVec);
	float3 intersecAtMaxPlane = (bboxMax - vertexPos) * invOrigVec;
	float3 intersecAtMinPlane = (bboxMin - vertexPos) * invOrigVec;
	float3 largestIntersec = max(intersecAtMaxPlane, intersecAtMinPlane);
	float Distance = min(min(largestIntersec.x, largestIntersec.y), largestIntersec.z);
	float3 IntersectPositionWS = vertexPos + origVec * Distance;
	float3 localCorrectedVec = IntersectPositionWS - cubemapPos;
	//localCorrectedVec.z /=  (bboxMax.z - bboxMin.z);
	return (localCorrectedVec);
}

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;
	float4 misc = Misc.Sample(MainSampler, uv);
	float3 Color = ColorTexture.Sample(MainSampler, uv).rgb;

	float4 wsPosition = Geometry.Sample(MainSampler, uv);
	float4 wsNormal = Normal.Sample(MainSampler, uv);

	Ray Reflection;
	Reflection.origin = wsPosition.xyz;
	Reflection.dir = reflect(wsPosition.xyz - cameraPos.xyz, wsNormal.xyz);

	float3 CubemapReflectionDir = Reflection.dir;
	if (cubemapPos.w > 0.0f)
		CubemapReflectionDir = LocalCorrect(CubemapReflectionDir.xyz, cubemapBboxA.xyz, cubemapBboxB.xyz, wsPosition.xyz, cubemapPos.xyz);

	float2 ssrPos = SSRPos.Sample(MainSampler, uv).xy;
	float3 reflColor = SSRTexture.SampleLevel(MainSampler, uv, misc.ROUGHNESS * 4).rgb;
	if (ssrPos.x > 1.0f) {
		reflColor = SampleReflection(Reflection.dir, misc.ROUGHNESS * 4);
	}
	Output.Color = float4(Color.rgb + reflColor * misc.SPECULAR * misc.AOVAL, 1.0f);
	
	return Output;
}