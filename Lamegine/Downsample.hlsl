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
Texture2D Tex				: register(t0);

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 xy = 1 / resolution.xy;
	Output.Color.rgba = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float2 off = float2(4.0f / 3.0f, 4.0f / 3.0f);
	Output.Color.rgb += Tex.Sample(SecondarySampler, input.Texcoord).rgb * 0.29411764705882354;
	Output.Color.rgb += Tex.Sample(SecondarySampler, input.Texcoord + off * xy).rgb * 0.35294117647058826;
	Output.Color.rgb += Tex.Sample(SecondarySampler, input.Texcoord - off * xy).rgb * 0.35294117647058826;

	return Output;
}