#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

struct PS_OUTPUT {
	float3 Color		: SV_TARGET;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D ColorTexture		: register(t0);
Texture2D SSRTexture		: register(t1);

//#define USE_FRESNEL
#include "FxLibrary.hlsli"

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	Output.Color.rgb = float3(0.0f, 0.0f, 0.0f);
	float2 uv = input.Texcoord;

	float2 nuv = SSRTexture.Sample(MainSampler, uv).xy;

	if (nuv.x >= 0.0f && nuv.y >= 0.0f && nuv.x < 1.0f && nuv.y < 1.0f) {
		Output.Color = ColorTexture.Sample(MainSampler, nuv);
	}// else discard;

	return Output;
}