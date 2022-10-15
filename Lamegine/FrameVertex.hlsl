#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT Output;
	Output.Position = float4(input.pos, 1.0f);
	Output.Texcoord = float2(input.texcoord.x, input.texcoord.y) * globalQuality;
	Output.Normal = input.normal;
	return Output;
}