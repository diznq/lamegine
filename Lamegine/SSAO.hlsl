#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

struct PS_OUTPUT {
	float Color		: SV_TARGET;
};

SamplerState MainSampler	: register(s0);

Texture2D Depth				: register(t0);
Texture2D Normal			: register(t1);
Texture2D Geometry			: register(t2);


PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;
	float depth = Depth.Sample(MainSampler, uv).r;
	float3 normal = Normal.Sample(MainSampler, uv).rgb;
	float4 position = mul(float4(Geometry.Sample(MainSampler, uv).rgb, 1.0f), view);
	//position.z /= position.w;
	
	const int samples = 16;
	float3 sample_sphere[samples] = {
		float3(0.5381, 0.1856,-0.4319), float3(0.1379, 0.2486, 0.4430),
		float3(0.3371, 0.5679,-0.0057), float3(-0.6999,-0.0451,-0.0019),
		float3(0.0689,-0.1598,-0.8547), float3(0.0560, 0.0069,-0.1843),
		float3(-0.0146, 0.1402, 0.0762), float3(0.0100,-0.1924,-0.0344),
		float3(-0.3577,-0.5301,-0.4358), float3(-0.3169, 0.1063, 0.0158),
		float3(0.0103,-0.5869, 0.0046), float3(-0.0897,-0.4940, 0.3287),
		float3(0.7119,-0.0154,-0.0918), float3(-0.0533, 0.0596,-0.5411),
		float3(0.0352,-0.0631, 0.5460), float3(-0.4776, 0.2847,-0.0271)
	};

	float AO = 0.0;

	float3 Div = float3(4.0f / resolution.xy, 1.0f / 2048.0f);

	for (int i = 0; i < 16; i++) {
		float3 samplePos = position.xyz + sample_sphere[i] * Div;
		float4 offset = float4(samplePos, 1.0);
		offset = mul(offset, proj);
		offset.xyz /= offset.w;
		offset.xy = (offset.xy + 1.0f) / 2.0f;
		offset.y = 1 - offset.y;

		float4 sampleDepth = mul(float4(Geometry.Sample(MainSampler, offset.xy).rgb, 1.0f), view);
		//sampleDepth.z /= sampleDepth.w;
		if (abs(position.z - sampleDepth.z) < 0.5f) {
			AO += step(sampleDepth.z, samplePos.z);
		}
	}

	AO = AO / 16.0f;
	Output.Color = AO;
	return Output;
}