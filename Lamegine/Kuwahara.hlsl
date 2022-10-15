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

Texture2D ColorTexture		: register(t0);
Texture2D LUT				: register(t1);

#define INCLUDE_LUT

#include "FxLibrary.hlsli"

float4 Kuwahara(float _Radius, float2 uv) {
	float2 _MainTex_TexelSize = 1.0f / resolution.xy;
	float n = float((_Radius + 1) * (_Radius + 1));
	float4 col = ColorTexture.Sample(MainSampler, uv);

	float3 m[4];
	float3 s[4];

	for (int p = 0; p < 4; ++p) {
		m[p] = float3(0, 0, 0);
		s[p] = float3(0, 0, 0);
	}

	float4 R[4] = {
		{ -_Radius, -_Radius,       0,       0 },
		{ 0, -_Radius, _Radius,       0 },
		{ 0,        0, _Radius, _Radius },
		{ -_Radius,        0,       0, _Radius }
	};

	[fastopt] for (int k = 0; k < 4; ++k) {
		[fastopt] for (int j = R[k].y; j <= R[k].w; ++j) {
			[fastopt] for (int i = R[k].x; i <= R[k].z; ++i) {
				float3 c = ColorTexture.Sample(MainSampler, uv + (float2(i * _MainTex_TexelSize.x, j * _MainTex_TexelSize.y))).rgb;
				m[k] += c;
				s[k] += c * c;
			}
		}
	}

	float min = 1e+2;
	float s2;
	for (k = 0; k < 4; ++k) {
		m[k] /= n;
		s[k] = abs(s[k] / n - m[k] * m[k]);

		s2 = s[k].r + s[k].g + s[k].b;
		if (s2 < min) {
			min = s2;
			col.rgb = m[k].rgb;
		}
	}
	return col;
}

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;

	Output.Color = Kuwahara(8, uv);
	Output.Color = Lookup(Output.Color);

	return Output;
}