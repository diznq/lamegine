struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

struct PS_OUTPUT {
	float4 Color		: SV_TARGET0;
	float4 ColorCopy	: SV_TARGET1;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

//#define MSAA
#ifdef MSAA
Texture2DMS<float4> Albedo			: register(t0);
#else
Texture2D Albedo			: register(t0);
#endif
Texture2D Normal			: register(t1);
Texture2D Geometry			: register(t2);
Texture2D Misc				: register(t3);
Texture2D Depth				: register(t4);
Texture2D ShadowMask0		: register(t5);
Texture2D Radiosity			: register(t6);
Texture2D ReflMap			: register(t7);
Texture2D AOTexture			: register(t8);
Texture2D Irradiance		: register(t9);
Texture2D SpecularBRDFLUT	: register(t10);

//#define USE_PBR

#include "Lighting.hlsli"

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
#ifndef USE_PBR
		float3 Specular;
		float2 LightingInfo;
		float3 shadowColor = lerp(skyColor.rgb * (1.0f - sunColor.rgb), skyColor.rgb, 0.5f);
		Output.Color = PostFxColor(input, LightingInfo, Specular);

		float Shadowness = LightingInfo.r;
		float FogRatio = LightingInfo.g;

		Output.Color = float4(Output.Color.rgb * Shadowness + Specular, 1.0f);
		Output.Color.rgb = lerp(Output.Color.rgb, fogSettings.rgb * sunColor.rgb * skyColor.rgb, FogRatio);

		Output.Color.rgb += Specular;
#else
		PBR_INPUT pbr_in;
		float2 uv = input.Texcoord;
		uint FrustumID = 0;
		float ShadowMin = shadowStrength;
		float FogDensity = fogSettings.a;
		pbr_in.uv = uv;
		pbr_in.albedo = Albedo.Sample(MainSampler, uv).xyz;
		pbr_in.ddna = Normal.Sample(MainSampler, uv);
		pbr_in.position = Geometry.Sample(MainSampler, uv).xyz;
		pbr_in.pbr = Misc.Sample(MainSampler, uv);
		pbr_in.reflection = reflect(pbr_in.position.xyz - cameraPos.xyz, pbr_in.ddna.xyz);
		pbr_in.dir = normalize(cameraPos.xyz - pbr_in.position.xyz);
		pbr_in.cosine = dot(pbr_in.dir, pbr_in.ddna.xyz);
		pbr_in.irradiance = SampleIrradiancePBR(pbr_in.ddna.xyz).xyz;
		pbr_in.invShadow = clamp(pow(CalcShadow(pbr_in.position.xyz, uv, FrustumID), 4), ShadowMin, 1.0f);
		pbr_in.depth = Depth.Sample(MainSampler, uv).x;
		float Distance = distance(pbr_in.position.xyz, cameraPos.xyz);
		float FogRatio = 1.0f - clamp(exp(-pow(FogDensity * Distance, 2.0)), 0.0f, 1.0f);
		pbr_in.fog = FogRatio;
		PBR_OUTPUT pbr = PBRColor(pbr_in);
		Output.Color = float4(pbr.color, 1.0f);
#endif
	Output.ColorCopy = Output.Color;
	
	return Output;
}