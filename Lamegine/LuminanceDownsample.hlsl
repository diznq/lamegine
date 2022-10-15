#include "Constants.hlsli"

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

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;
	float2 xy = float2(1.0f, 1.0f) / (resolution * 4.0f);
	Output.Color = float2(0.0f, 0.0f);
	
	[unroll] for (int x = 0; x < 2; x++) {
		[unroll] for (int y = 0; y < 2; y++) {
			Output.Color = max(Output.Color, ColorTexture.Sample(SecondarySampler, (uv + xy * float2(x, y))).rg);
		}
	}
	//Output.Color *= 0.25f;
	return Output;
}