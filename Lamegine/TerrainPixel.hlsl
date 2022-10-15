#include "Constants.h"

struct PS_INPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D Diffuse			: register(t0);
Texture2D Ambient			: register(t1);
Texture2D Bump				: register(t2);
Texture2D Misc				: register(t3);
Texture2D Alpha				: register(t4);
Texture2D Diffuse2			: register(t5);
Texture2D Bump2				: register(t6);
Texture2D Misc2				: register(t7);

#include "FxLibrary.hlsli"

struct PS_OUTPUT {
	float4 Color	: SV_TARGET0;
	float4 Normal	: SV_TARGET1;
	float Depth		: SV_TARGET2;
	float4 Geometry	: SV_TARGET3;
	float4 MiscInfo : SV_TARGET4;
};

#define INSTANCE_INDEX 0

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT Output;

	float d = Alpha.Sample(MainSampler, input.Texcoord).r;
	if (d < 0.5) discard;
	Output.Color.a = 1.0f;

	float4 Bump1Info = Bump.Sample(MainSampler, input.Texcoord);
	float4 Bump2Info = Bump2.Sample(MainSampler, input.Texcoord);

	Output.Color.rgb = Diffuse.Sample(MainSampler, (input.Texcoord)).rgb * instances[INSTANCE_INDEX].diffuse.rgb;
	Output.Normal = GetBumpNormal(input.Normal, input.Tangent, input.Texcoord, Bump1Info);
	Output.Depth = input.Position.z;
	Output.Geometry = float4(input.WSPosition, 1.0f);
	Output.MiscInfo = Misc.Sample(MainSampler, (input.Texcoord));

	//Output.Color.rgb = Output.Normal.rgb;

	return Output;
}
