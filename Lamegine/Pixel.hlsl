#include "Constants.h"

struct PS_INPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
	uint InstanceID : Output;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D Diffuse			: register(t0);
Texture2D Bump				: register(t1);
Texture2D Misc				: register(t2);
Texture2D Ambient			: register(t3);
Texture2D Alpha				: register(t4);

Texture2D Diffuse2			: register(t5);
Texture2D Bump2				: register(t6);
Texture2D Misc2				: register(t7);

Texture2D Diffuse3			: register(t8);
Texture2D Bump3				: register(t9);
Texture2D Misc3				: register(t10);

#include "FxLibrary.hlsli"

struct PS_OUTPUT {
	float4 Color	: SV_TARGET0;
	float4 Normal	: SV_TARGET1;
	float4 Geometry	: SV_TARGET2;
	float4 MiscInfo : SV_TARGET3;
	//float Depth : SV_TARGET4;
};

#define INSTANCE_INDEX input.InstanceID

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT Output;

	float2 txCoord = input.Texcoord * instances[INSTANCE_INDEX].txScale;
	float d = Alpha.Sample(MainSampler, txCoord).r;

	float4 Diff1 = Diffuse.Sample(MainSampler, txCoord);
	float a = min(d, Diff1.w);
	if (a < 0.33f) discard;
	if (a < 0.9f) {
		float mn = min(d, Diff1.w);
		uint2 pixel = uint2(input.Position.xy);
		if ((pixel.x + pixel.y) & 2) discard;
	}
	Output.Color.a = 1.0f;
	float4 Bump1Info = Bump.Sample(MainSampler, txCoord);
	Bump1Info.w = 1.0f;
	float4 Misc1Info = Misc.Sample(MainSampler, txCoord);

	Output.Color.rgba = Diff1.rgba * float4(instances[INSTANCE_INDEX].diffuse.rgb, 1.0f);
	Output.Normal = GetBumpNormal(input.Normal, input.Tangent, txCoord, Bump1Info);
	Output.Geometry = float4(input.WSPosition, 1.0f);
	Output.MiscInfo = Misc1Info * instances[INSTANCE_INDEX].pbrInfo;

	return Output;
}
