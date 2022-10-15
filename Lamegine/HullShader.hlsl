#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
	float TessFactor	: TESS;
};

struct HS_OUTPUT
{
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
};

struct HS_PATCH_OUT {
	float Edges[3]		: SV_TessFactor;
	float Inside		: SV_InsideTessFactor;
};

HS_PATCH_OUT ConstantHS(InputPatch<VS_OUTPUT, 3> patch, uint PatchID : SV_PrimitiveID)
{
	HS_PATCH_OUT pt;
	pt.Edges[0] = 0.5f*(patch[1].TessFactor + patch[2].TessFactor);
	pt.Edges[1] = 0.5f*(patch[2].TessFactor + patch[0].TessFactor);
	pt.Edges[2] = 0.5f*(patch[0].TessFactor + patch[1].TessFactor);
	pt.Inside = pt.Edges[0];
	return pt;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> inputPatch, uint uCPID : SV_OutputControlPointID, uint uPID : SV_PrimitiveID)
{
	HS_OUTPUT Out;
	Out.WSPosition	=	inputPatch[uCPID].WSPosition;
	Out.Texcoord	=	inputPatch[uCPID].Texcoord;
	Out.Normal		=	inputPatch[uCPID].Normal;
	Out.Tangent		=	inputPatch[uCPID].Tangent;
	return Out;
}