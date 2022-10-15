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


PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;
	float2 xy = globalQuality / resolution.xy;
	Output.Color = ColorTexture.Sample(SecondarySampler, uv);
	const int power = 31;
	const float kernel[power] = {
		0.005913, 0.007946, 0.010461, 0.013495, 0.017058, 0.021127, 0.025639, 0.030486,
		0.035519, 0.040549, 0.045357, 0.049712, 0.053386, 0.056176, 0.057919, 0.058512,
		0.057919, 0.056176, 0.053386, 0.049712, 0.045357, 0.040549, 0.035519, 0.030486,
		0.025639, 0.021127, 0.017058, 0.013495, 0.010461, 0.007946, 0.005913
	};
	Output.Color.rgba = float4(0.0f, 0.0f, 0.0f, 1.0f);
	[unroll] for (int i = 0; i < power; i++) {
		Output.Color.rgb += ColorTexture.Sample(SecondarySampler, saturate(uv + float2(i - power / 2, 0.0f) * xy)).rgb * kernel[i];
	}
	return Output;
}