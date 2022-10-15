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
Texture2D FeedbackTexture	: register(t1);

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	//uint2 pos = (uint2)floor(abs(input.Texcoord * resolution.xy));
	float3 Color = ColorTexture.Sample(MainSampler, input.Texcoord).rgb;
	//float3 Feedback = FeedbackTexture.Sample(MainSampler, input.Texcoord).rgb;
	Output.Color.a = 1.0f;	
	Output.Color.rgb = Color;

	return Output;
}