#include "Constants.hlsli"
#include "FxLibrary.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

struct PS_OUTPUT {
	float2 Color		: SV_TARGET;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D ColorTexture		: register(t0);
Texture2D AlbedoTexture		: register(t1);
Texture2D LastLuminance		: register(t2);

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord * realGlobalQuality;
	float2 xy = (float2(1.0f, 1.0f) / resolution) * realGlobalQuality;
	Output.Color = float2(0.0f, 0.0f);
	[unroll] for (int x = 0; x < 2; x++) {
		[unroll] for (int y = 0; y < 2; y++) {
			float3 color = ColorTexture.Sample(SecondarySampler, uv + xy * float2(x, y)).rgb;
			float3 albedo = AlbedoTexture.Sample(SecondarySampler, uv + xy * float2(x, y)).rgb;
			albedo.rgb *= albedo.rgb;
			float baseLum = GetLuminance(albedo);
			if (baseLum == 0.0f) baseLum = 0.2f;
			if (baseLum < 0.1f) baseLum = 0.1f;
			float lum = GetLuminance(color);
			
			Output.Color.xy += float2(
				log(lum + 1e-6),
				log(lum/baseLum * 3.1428 + 1e-6)
			);
		}
	}
	Output.Color *= 0.25f;
	Output.Color.xy = lerp(Output.Color.xy, LastLuminance.Sample(SecondarySampler, uv).xy, 0.98f);
	return Output;
}