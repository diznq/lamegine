#include "Constants.hlsli"

struct DS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
};

struct HS_OUTPUT
{
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
};

struct HS_PATCH_OUT
{
	float Edges[3]			: SV_TessFactor;
	float Inside			: SV_InsideTessFactor;
};

SamplerState MainSampler	: register(s0);
Texture2D Normal			: register(t0);

[domain("tri")]
DS_OUTPUT main(HS_PATCH_OUT input, float3 domain : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 3> patch)
{
	DS_OUTPUT Output;

	Output.Texcoord		=	float2(
							  patch[0].Texcoord*domain.x
							+ patch[1].Texcoord*domain.y
							+ patch[2].Texcoord*domain.z
							);
	Output.Normal		=	float3(
							  patch[0].Normal*domain.x
							+ patch[1].Normal*domain.y
							+ patch[2].Normal*domain.z
							);
	Output.Tangent		=	float3(
							  patch[0].Tangent*domain.x
							+ patch[1].Tangent*domain.y
							+ patch[2].Tangent*domain.z
							);
	Output.WSPosition	=	float3(
							  patch[0].WSPosition*domain.x
							+ patch[1].WSPosition*domain.y
							+ patch[2].WSPosition*domain.z
							);

	float4 info = Normal.SampleLevel(MainSampler, Output.Texcoord.xy, 0);
	Output.WSPosition += Output.Normal * (info.b - 0.5f) * 0.25f;

	float4 pos = float4(Output.WSPosition, 1.0f);
	//pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, proj);

	Output.Position = pos;

	return Output;
}
