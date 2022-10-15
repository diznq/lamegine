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
Texture2D DepthTexture		: register(t1);
Texture2D GeometryTexture	: register(t2);

#include "FxLibrary.hlsli"

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;
	float2 xy = globalQuality / resolution.xy;

	float dstCenter = distance(cameraPos, GeometryTexture.Sample(SecondarySampler, float2(0.5f, 0.5f) * globalQuality));
	float dst = distance(cameraPos, GeometryTexture.Sample(SecondarySampler, uv));
	//uv += shift;
	Output.Color = ColorTexture.Sample(SecondarySampler, uv);

	float diff = dst - dstCenter;

	if (diff > 5.0f) {
		int power = 8;// (int)(clamp((diff - 5.0f) / 2, 1.0f, 8.0f));
		float part = 1.0f / (power * power);
		float3 original = Output.Color.rgb;
		Output.Color.rgb = float3(0.0f, 0.0f, 0.0f);
		[unroll] for (int x = 0; x < power; x++) {
			[unroll] for (int y = 0; y < power; y++) {
				Output.Color.rgb += ColorTexture.Sample(SecondarySampler, uv + float2(x-power/2, y-power/2) * xy).rgb * part;
			}
		}
		Output.Color.rgb = lerp(original, Output.Color.rgb, saturate((diff - 5.0f) / 5.0f));
	}

	return Output;
}