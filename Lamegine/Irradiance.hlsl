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

SamplerState MainSampler		: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D ReflMap			    : register(t0);
Texture2D EquiLookup			: register(t1);

#define PI 3.14159265359

float4 SampleReflection(float3 dir, float rougness)
{
    dir = normalize(dir);
    half theta = acos(dir.z); // +z is up
    half phi = atan2(-dir.y, -dir.x) + PI;
    half2 uv = half2(phi, theta) * half2(0.1591549, 0.31830988618);
    return ReflMap.SampleLevel(MainSampler, uv, 2);
}

float3 GetNormal(float2 uv) {
    uv.y = 1.0f - uv.y;
    return EquiLookup.Sample(MainSampler, uv).rgb;
}

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	Output.Color = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float2 uv = input.Texcoord;
    float3 normal = GetNormal(uv);
    float3 up = float3(0.0f, 0.0f, 1.0);
    float3 right = cross(up, normal);
    up = cross(normal, right);

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += SampleReflection(normalize(sampleVec), 0.0f).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    Output.Color = float4(PI * irradiance * (1.0 / float(nrSamples)), 1.0f);
    //Output.Color.xyz = normal;

	return Output;
}