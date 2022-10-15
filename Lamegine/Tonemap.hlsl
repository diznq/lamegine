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

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D ColorTexture		: register(t0);
Texture2D BloomTexture		: register(t1);
Texture2D LuminanceTexture	: register(t2);
Texture2D LensTexture		: register(t3);

#include "FxLibrary.hlsli"

float ExposureSettings(float aperture, float shutterSpeed, float sensitivity) {
	return log2((aperture * aperture) / shutterSpeed * 100.0 / sensitivity);
}

float Exposure(float ev100) {
	return 1.0 / (pow(2.0, ev100) * 1.2);
}

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;

	float2 Lum = LuminanceTexture.Sample(SecondarySampler, uv * (1.0f / globalQuality)).xy;
	float3 color = ColorTexture.Sample(SecondarySampler, uv).rgb;
	float3 bloom = BloomTexture.Sample(SecondarySampler, uv).rgb * bloomStrength;
	float3 now = color + bloom;


	float dyn_exposure = exposure / clamp(Lum.y, 1.0f, 100.0f);
	now = float3(1.0f, 1.0f, 1.0f) - exp(-now * dyn_exposure);
	now = pow(abs(now), float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));

#ifdef LENS_FLARE
	float4 sunPos = TranslateRay(cameraPos.xyz + sunDir.xyz * 1000);

	const float uGhostDispersal = 0.4f;
	float2 ghostVec = (sunPos.xy - uv) * uGhostDispersal;
	const int uGhosts = 3;
	float2 nrm = normalize(resolution);
	ghostVec.yx *= nrm;

	float4 Lens = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float dst = distance(sunPos.xy * nrm, uv.xy * nrm);
	for (int i = 0; i < uGhosts; ++i) {
		float2 offset = frac(uv + ghostVec * float(i));
		float4 n = BloomTexture.Sample(MainSampler, offset);
		Lens += n * GetLuminance(n.rgb) * (1.0f / uGhosts);
	}
	Lens *= LensTexture.Sample(MainSampler, dst);
	now += Lens * 0.25f;
#endif
	Output.Color.a = 1.0f;
	//Output.Color.rgb = now;
	//if(uv.x < 0.5f)
	Output.Color.rgb = ((now - hdrSettings.x) * hdrSettings.y);
	Output.Color = clamp(Output.Color, 0.0f, 1.0f);
	return Output;
}