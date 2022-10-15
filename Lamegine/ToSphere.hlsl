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

Texture2D Cubemap0			: register(t0);
Texture2D Cubemap1			: register(t1);
Texture2D Cubemap2			: register(t2);
Texture2D Cubemap3			: register(t3);
Texture2D Cubemap4			: register(t4);
Texture2D Cubemap5			: register(t5);

//#define USE_FRESNEL
#include "FxLibrary.hlsli"

#define PI 3.14159265359

float3 CubeSample(float3 v) {
	float3 vAbs = abs(v);
	float ma;
	float2 uv;
	uint faceIndex = 0;
	float2 fuv;
	if (vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
	{
		faceIndex = v.z < 0.0 ? 5.0 : 4.0;
		ma = 0.5 / vAbs.z;
		uv = float2(v.z < 0.0 ? -v.x : v.x, -v.y);
		fuv = uv * ma + 0.5;
		if (v.z > 0.0) {
			fuv.y = 1.0 - fuv.y;
			fuv.x = 1.0 - fuv.x;
		}
	}
	else if (vAbs.y >= vAbs.x)
	{
		faceIndex = v.y < 0.0 ? 2.0 : 3.0;
		ma = 0.5 / vAbs.y;
		uv = float2(v.x, v.y < 0.0 ? -v.z : v.z);
		fuv = uv * ma + 0.5;
		if (v.y < 0.0f) {
			fuv.y = 1.0f - fuv.y;
			fuv.x = 1.0f - fuv.x;
		}
	}
	else
	{
		faceIndex = v.x < 0.0 ? 1.0 : 0.0;
		ma = 0.5 / vAbs.x;
		uv = float2(v.x < 0.0 ? v.z : -v.z, -v.y);
		fuv = uv * ma + 0.5;
		fuv.xy = fuv.yx;
		if (v.x > 0.0f) {
			fuv.y = 1.0f - fuv.y;
		} else
			fuv.x = 1.0f - fuv.x;
	}
	if (faceIndex == 0) return Cubemap0.Sample(MainSampler, fuv).rgb;
	if (faceIndex == 1) return Cubemap1.Sample(MainSampler, fuv).rgb;
	if (faceIndex == 2) return Cubemap2.Sample(MainSampler, fuv).rgb;
	if (faceIndex == 3) return Cubemap3.Sample(MainSampler, fuv).rgb;
	if (faceIndex == 4) return Cubemap4.Sample(MainSampler, fuv).rgb;
	if (faceIndex == 5) return Cubemap5.Sample(MainSampler, fuv).rgb;
	return float3(0.0f, 0.0f, 0.0f);
}

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	Output.Color = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float2 uv = input.Texcoord;
	uv.x = 1.0f - uv.x;

	float2 a = (uv * 2 - 1.0f) * float2(3.14159265, 1.57079633);
	float2 c = cos(a), s = sin(a);
	
	Output.Color.rgb = CubeSample(float3(float2(s.x, c.x) * c.y, s.y));
	return Output;
}