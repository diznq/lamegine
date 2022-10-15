struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D Albedo			: register(t0);
Texture2D Normal			: register(t1);
Texture2D Misc				: register(t2);
Texture2D Ambient			: register(t3);
Texture2D Alpha				: register(t4);

Texture2D Diffuse2			: register(t5);
Texture2D Normal2			: register(t6);
Texture2D Misc2				: register(t7);

Texture2D Diffuse3			: register(t8);
Texture2D Normal3			: register(t9);
Texture2D Misc3				: register(t10);

Texture2D ShadowMask0		: register(t5);
Texture2D Self				: register(t6);
Texture2D SelfLight			: register(t7);
Texture2D Radiosity			: register(t7);
Texture2D ReflMap			: register(t8);
Texture2D Irradiance		: register(t9);
Texture2D SpecularBRDFLUT   : register(t10);

#define FORWARD
#define INSTANCE_INDEX 0
#include "Lighting.hlsli"

#ifdef POOR_REFRACTION
struct PS_OUTPUT {
	float4 Color		: SV_TARGET0;
	float4 Distortion	: SV_TARGET1;
};
#else
struct PS_OUTPUT {
	float4 Color		: SV_TARGET0;
	float4 Normals		: SV_TARGET1;
	float4 Geometry		: SV_TARGET2;
	//float Depth			: SV_TARGET4;
};
#endif


PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 shadowness = float2(1.0f, 0.0f);
	float2 uv = (input.Position.xy / (resolution.xy));
#ifdef POOR_REFRACTION
	float2 offset = (Normal.Sample(MainSampler, input.Texcoord).xy - 0.5f)*2.0f;
	float3 Specular = float3(0.0f, 0.0f, 0.0f);
	Output.Color = PostFxColor(input, shadowness, Specular) * instances[INSTANCE_INDEX].diffuse;
	Output.Color.rgb += Specular;
	Output.Color.a = 0.5f; // alpha + GetLuminance(Specular);
	Output.Distortion = float4(offset.xy, 1 - alpha, 0.5f);
#else
	float2 texCoord = input.Texcoord * instances[INSTANCE_INDEX].txScale;
	float2 offset = Normal.Sample(MainSampler, texCoord).xy - 0.5f;
	uv.xy += (offset / 4.0f);
	uv = saturate(uv) * globalQuality;

	float3 Specular = float3(0.0f, 0.0f, 0.0f);
	float4 albedo = Albedo.Sample(MainSampler, texCoord);
	float A = instances[INSTANCE_INDEX].diffuse.w;
	float4 base = Self.Sample(SecondarySampler, uv);
	if (instances[INSTANCE_INDEX].pbrExtra.x > 0.25f) {
		float4 nbase = float4(0.0f, 0.0f, 0.0f, 1.0f);
		for (int jy = -2; jy <= 2; jy++) {
			for (int jx = -2; jx <= 2; jx++) {
				float2 uw = uv + instances[INSTANCE_INDEX].pbrExtra.x * float2(jx, jy) / (resolution / globalQuality);
				nbase += Self.Sample(SecondarySampler, uw);
			}
		}
		base = lerp(base, nbase / 25.0f, instances[INSTANCE_INDEX].pbrExtra.y);
	}

	float3 shadowColor = lerp(skyColor.rgb * (1.0f - sunColor.rgb), skyColor.rgb, 0.5f);

	Output.Color = lerp(base * albedo * instances[INSTANCE_INDEX].diffuse, PostFxColor(input, shadowness, Specular) * instances[INSTANCE_INDEX].diffuse, A) + Ambient.Sample(MainSampler, texCoord) * A * instances[INSTANCE_INDEX].ambient;
	
	float Shadowness = shadowness.r;
	float FogRatio = shadowness.g;

	//Output.Color.rgb = Output.Color.rgb * lerp(sunColor.rgb, shadowColor.rgb * shadowStrength, 1 - Shadowness);
	Output.Color.rgb = lerp(Output.Color.rgb, fogSettings.rgb * sunColor.rgb * skyColor.rgb, FogRatio);
	
	Output.Color.rgb += Specular;
	Output.Color.a = 0.5f;
	Output.Geometry = float4(input.WSPosition.xyz, 1.0f);
	Output.Normals = GetBumpNormal(input.Normal, input.Tangent, texCoord, Normal.Sample(MainSampler, texCoord));
#endif
	return Output;
}