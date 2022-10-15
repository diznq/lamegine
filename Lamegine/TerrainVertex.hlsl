#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
	uint InstanceID		: Output;
};


SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D Heightmap			: register(t0);
Texture2D Normal			: register(t1);

float height(float2 uv) {
	return Heightmap.SampleLevel(MainSampler, uv, 0).r;
}

float3 GetTerrainNormal(float2 P) {
	float3 off = float3(1.0, 1.0, 0.0) / 1024.0f;
	float hL = height(P.xy - off.xz);
	float hR = height(P.xy + off.xz);
	float hD = height(P.xy - off.zy);
	float hU = height(P.xy + off.zy);

	float3 N = float3(1.0f, 1.0f, 1.0f);
	N.x = hL - hR;
	N.y = hD - hU;
	N.z = 2.0;
	N = normalize(N);
	return N;
}

#define INSTANCE_INDEX input.InstanceID

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT Output;

	float4 pos = float4(input.pos, 1.0f);
	//float2 uv = (pos.xy);
	pos = mul(pos, instances[INSTANCE_INDEX].model);
	float2 uv = pos.xy / 1024.0f;
	pos.z *= height(uv);

	float4 realPos = pos;

	pos = mul(pos, view);
	pos = mul(pos, proj);

	float3 normal = Normal.SampleLevel(MainSampler, uv, 0).rgb;

	Output.Position = pos;
	Output.Texcoord = input.pos.xy;
	Output.Normal = mul(float4(normal, 0.0f), instances[INSTANCE_INDEX].model).rgb;
	Output.Tangent = mul(input.tangent, (float3x3)instances[INSTANCE_INDEX].model);
	Output.WSPosition = realPos.xyz;
	Output.InstanceID = input.InstanceID;

	return Output;
}
