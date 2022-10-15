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
Texture2D LUT				: register(t1);

#define INCLUDE_LUT

#include "FxLibrary.hlsli"

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float4 Color = ColorTexture.Sample(MainSampler, input.Texcoord * realGlobalQuality);
	if (distortionStrength > 0.0f) {
		float2 Offset = float2(32.0f * distortionStrength / resolution.x, 0.0f);
		float R = ColorTexture.Sample(MainSampler, (input.Texcoord + Offset) * realGlobalQuality).r;
		float B = ColorTexture.Sample(MainSampler, (input.Texcoord - Offset) * realGlobalQuality).b;
		Color.r = R;
		Color.b = B;
	}
	Output.Color = Color; // Lookup(Color);
	float2 adjusted = input.Texcoord.xy - 0.5f;
	adjusted = adjusted * normalize(resolution);

	float vignette = distance(input.Texcoord, float2(0.5f, 0.5f)) * 0.7;

	if (distance(adjusted, float2(0.0f, 0.0f)) <= 2.0f / resolution.x) {
		Output.Color.rgb = float3(1.0f, 1.0f, 1.0f);
	}

	//Output.Color.rgb *= smoothstep(1.0f, 0.1f, vignette);
	return Output;
}