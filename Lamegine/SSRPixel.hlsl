#include "Constants.hlsli"

struct VS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
};

struct PS_OUTPUT {
	float2 Color		: SV_TARGET;
};

SamplerState MainSampler	: register(s0);
SamplerState SecondarySampler	: register(s1);

Texture2D Normal			: register(t0);
Texture2D Depth				: register(t1);
Texture2D Geometry			: register(t2);
Texture2D SSRHistory		: register(t3);

#define INCLUDE_SSR
#define BINARY_SEARCH
#define BINARY_SEARCH_DEPTH 4
#include "FxLibrary.hlsli"


#ifdef INCLUDE_SSR
float GetMinDepth(float2 pos) {
	float minimum = 2.0f;
	[unroll] for (int i = -1; i <= 1; i++) {
		[unroll] for (int j = -1; j <= 1; j++) {
			float depth = Depth.Sample(MainSampler, pos + (i / resolution.x, j / resolution.y)).r;
			if (depth < minimum) minimum = depth;
		}
	}
	return minimum;
}

float2 SSR(Ray r, in float cmp, in float stride, in float strideLinear, in float3 wsNormal) {
	float3 prevRay = r.origin;
	float3 ray = r.origin;
	bool found = false;
	float4 pos;
	float depth = 0.0f;
	float3 nrm = 0.0f;
	[unroll]
	for (int i = 1; i <= 8; i++) {
		ray = r.origin + r.dir * i * stride;
		pos = TranslateRay(ray);
		if (pos.x<0.0f || pos.x>1.0f || pos.y<0.0f || pos.y>1.0f || pos.w <= 0.0f) {
			return float2(3.0f, 3.0f);
		}
		depth = Depth.Sample(MainSampler, pos.xy).r;
		nrm = Normal.Sample(MainSampler, pos.xy).xyz;
		float angl = (dot(nrm, wsNormal));
		//if (depth == 0.0f) depth = 1.0f;

		if (depth < pos.z && angl < 0.5f) {
			found = true;
			break;
		}

		stride += strideLinear;
		prevRay = ray;
	}

	if (found) {

#ifdef BINARY_SEARCH
		[unroll]
		for (int j = 0; j < BINARY_SEARCH_DEPTH; j++) {
			float3 avg = lerp(prevRay, ray, 0.5f);
			pos = TranslateRay(avg);
			if (pos.x<0.0f || pos.x>1.0f || pos.y<0.0f || pos.y>1.0f || pos.w <= 0.0f) return float2(3.0f, 3.0f);

			depth = Depth.Sample(MainSampler, pos.xy).r;
			if (depth == 0.0f) depth = 1.0f;
			if (abs(depth - pos.z) < cmp) return pos.xy;

			if (depth < pos.z) ray = avg;
			else prevRay = avg;
		}
#endif

		return pos.xy;

	}

	return float2(2.0f, 2.0f);
}
#endif


PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT Output;
	float2 uv = input.Texcoord;

	float4 vsDepth = Depth.Sample(MainSampler, uv);
	Output.Color = float2(2.0f, 2.0f);

	/*uint2 px = uint2(uv * resolution);
	uint pid = px.y + px.x;
	
	if (frame & 1) {
		if (pid & 1) discard;
	} else {
		if (!(pid & 1)) discard;
	}*/

	if (vsDepth.r > 0.0f) {
		float4 wsPosition = Geometry.Sample(MainSampler, uv);
		//float dist = clamp(distance(wsPosition.xyz, cameraPos.xyz), 1.0f, 500.0f);
		float3 wsNormal = Normal.Sample(MainSampler, uv).xyz;
		//if ((wsNormal.z) > 0.5f && abs(wsNormal.x) < 0.25f && abs(wsNormal.y) < 0.25f) {
			Ray Reflection;
			Reflection.origin = wsPosition.xyz;
			//float dist = 0.1f * sin(62.8 * time);
			//float3 distortion = float3(0.0f, 0.0f, dist);
			Reflection.dir = reflect(normalize(wsPosition.xyz - cameraPos.xyz), wsNormal.xyz);
			float stride = 1.0f;
			float strideLinear = 1.25f;
			Output.Color = SSR(Reflection, 0.0001f, stride, strideLinear, wsNormal.xyz);
			//if (Output.Color.r > 0.0f && Output.Color.r<1.0f && Output.Color.g > 0.0f && Output.Color.g < 1.0f) {
				//float3 wsNormal2 = Normal.Sample(MainSampler, Output.Color.xy).xyz;
				//float cosx = acos(clamp(dot(wsNormal.xyz, wsNormal2.xyz), -1.0f, 1.0f));
				//if (abs(cosx) < 1.0f)
				//	discard;// Output.Color.rg = float2(2.0f, 2.0f);
			//} else {
			//	discard;
			//}
		//}
	}

	return Output;
}