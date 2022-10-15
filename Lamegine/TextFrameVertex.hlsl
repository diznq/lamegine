#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
	float3 Color		: TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT Output;
	float2 info = letters[input.InstanceID].position;
	uint2 uv = letters[input.InstanceID].id;
	Output.Color = letters[input.InstanceID].color.rgb;

	float fontSize = 0.7f;

	float2 pos = (input.pos.xy + 1.0f) / 2.0f;
	pos *= fontSize * float2(18.0f, 26.0f) / resolution.xy;
	pos += info.xy;
	pos = (pos * 2.0f) - 1.0f;
	Output.Position = float4(pos.xy, 0.0f, 1.0f);

	float Xpart = 18.0f / 288.0f;
	float Ypart = 26.0f / 416.0f;

	float2 texUv = float2(uv.x * Xpart, uv.y * Ypart);
	Output.Texcoord = (float2(input.texcoord.x, input.texcoord.y) * float2(16.0f / 288.0f, 24.0f / 416.0f)) + texUv;
	input.texcoord.y = 1.0f - input.texcoord.y;
	Output.Normal = input.normal;
	return Output;
}