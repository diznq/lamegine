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

Texture2D ColorTexture		: register(t0);

#include "FxLibrary.hlsli"

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;
	float2 xy = 1.0f / resolution.xy;
	float3 now = ColorTexture.Sample(MainSampler, uv).rgb;
	float brightness = GetLuminance(now);

	Output.Color.rgba = float4(0.0f, 0.0f, 0.0f, 1.0f);

	if (brightness > 1.0f) {
		Output.Color.rgb = ColorTexture.Sample(MainSampler, uv).rgb;
	}

	return Output;
}