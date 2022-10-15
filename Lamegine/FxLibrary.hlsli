struct Ray
{
	float3 origin;
	float3 dir;
};

float GetLuminance(float3 c) {
	return dot(c, float3(0.2126f, 0.7152f, 0.0722f));
}

float4 TranslateRay(in float3 ray) {
	float4 pos = float4(ray, 1.0f);
	pos = mul(pos, view);
	pos = mul(pos, proj);
	pos.xyz /= pos.w;
	pos.xy = ((pos.xy + 1.0f) / 2.0f);
	pos.y = 1 - pos.y;
	return pos * realGlobalQuality;
}

float Linearize(in float z, in float n = 0.1f, in float f = 2048.0f) {
	return z / (f - z * (f - n));
}

float4 RGBToYuv(float4 rgba) {
	return mul(matrix(
		0.299, 0.587, 0.114, 0.0,
		-0.169, -0.331, 0.5, 0.5,
		0.5, -0.419, -0.091, 0.5,
		0.0, 0.0, 0.0, 1.0), rgba);
}

float4 YuvToRGB(float4 yuva) {
	return mul(matrix(
		1.014, 0.0239, 1.4017, -0.7128,
		0.9929, -0.3564, -0.7142, 0.5353,
		1.0, 1.7722, 0.001, -0.8866,
		0.0, 1.7722, 0.0, 1.0), yuva);
}

float4 GetBumpNormal(in float3 Normal0, in float3 Tangent0, in float2 Texcoord, in float4 BumpMapNormal) {
	float3 NormalRgb = normalize(Normal0);
	float3 Tangent = normalize(Tangent0);
	Tangent = normalize(Tangent - dot(Tangent, NormalRgb) * NormalRgb);
	float3 Bitangent = normalize(cross(Tangent, NormalRgb));
	BumpMapNormal.rgb = 2.0f * BumpMapNormal.rgb - float3(1.0f, 1.0f, 1.0f);
	float Intensity = 0.85f; // BumpMapNormal.b;
	//if (Intensity < 0.5f) Intensity = 0.0f;
	float3 Final = normalize(NormalRgb + Intensity * (BumpMapNormal.r * Tangent + BumpMapNormal.g * Bitangent));
	return float4(Final, BumpMapNormal.a);
}

float Random(in float2 uv)
{
	float2 noise = (frac(sin(dot(uv, float2(12.9898, 78.233)*2.0)) * 43758.5453));
	return abs(noise.x + noise.y) * 0.5;
}

void Fresnel(float3 I, float3 N, inout float ior, out float kr)
{
	float cosi = clamp(-1, 1, dot(I, N));
	float etai = 1, etat = ior;
	if (cosi > 0) {
		float tmp = etai;
		etai = etat;
		etat = tmp;
	}
	float sint = etai / etat * sqrt(max(0.f, 1 - cosi * cosi));
	// Total internal reflection
	if (sint >= 1) {
		kr = 1;
	}
	else {
		float cost = sqrt(max(0.0f, 1 - sint * sint));
		cosi = abs(cosi);
		float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
		float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
		kr = (Rs * Rs + Rp * Rp) / 2;
	}
}

float CalcLightRatio(float3 surfaceNormal, float3 pos, float3 lightDirection, float specularRatio, float shadowPower, out float specularOut) {
	float3 viewDirection = normalize(cameraPos.xyz - pos.xyz);
	surfaceNormal = normalize(surfaceNormal);
	lightDirection = normalize(-lightDirection);
	float light = 0.0f;
	float diffuse = saturate(dot(surfaceNormal, lightDirection));
	light += diffuse;
	specularOut = 0.0f;
	if (light > 0.0f) {
		float3 reflection = normalize(2 * light * surfaceNormal - lightDirection);
		float specularPower = 32;
		float specComponent = pow(clamp(dot(reflection, viewDirection), 0.0f, 1.0f), specularPower);
		specularOut = specComponent * specularRatio * shadowPower;
	}
	return saturate(light);
}

float3 CalcPointLightColor(float3 base, float intensity, float3 pos, float3 normal, float3 lightPosition, in out float3 Specular, float specular = 1.0f, float shadow = 1.0f) {
	float3 lightDirection = normalize(pos - lightPosition);
	float power = intensity / pow(distance(pos, lightPosition), 2.0f);
	float spec = 0.0f;
	float ratio = 0.0f;
	//if (power < 0.025f) return float3(0.0f, 0.0f, 0.0f);
	ratio = power * CalcLightRatio(normal, pos, lightDirection.xyz, specular, shadow, spec);
	Specular += power * base * spec;
	return base * ratio;
}

float3 CalcDirLightColor(float3 base, float intensity, float3 pos, float3 normal, float3 lightDirection, in out float3 Specular, float specular = 1.0f, float shadow = 1.0f) {
	float spec = 0.0f;
	float ratio = tanh(8 * CalcLightRatio(normal, pos, lightDirection, specular, shadow, spec));
	if (ratio > 0.08) ratio = 1.0;
	else ratio += shadowStrength;
	//if (ratio < 0.0125f) return float3(0.0f, 0.0f, 0.0f);
	Specular += base * spec;
	return base * intensity * ratio;
}

float CalcVarianceShadow(float moments, float distance, const float Vs)
{
	const float C = Vs;
	float z = moments.x;
	float d = distance;
	return pow(exp(C * z) * exp(-C * d), 16.0f);
}

bool InFrustum(float4 Pclip) {
	return abs(Pclip.x) < Pclip.w && abs(Pclip.y) < Pclip.w && 0 < Pclip.z && Pclip.z < Pclip.w;
}

#ifdef INCLUDE_SHADOWS

float CalcShadow(in float3 realPosition, in float2 UV, out uint FrustumID) {
	float result = 1.0f;
	//bool found = false;
	FrustumID = 10;

	float Close = 0.0f;
	float Prev = 1.0f;
	bool Transit = false;
	[unroll]
	for (uint i = 0; i < NUM_SHADOW_CASCADES; i++) {
		float4 ShadowPos = float4(realPosition, 1.0f);
		ShadowPos = mul(ShadowPos, shadowView[i]);
		ShadowPos = mul(ShadowPos, shadowProj[i]);

		if (!InFrustum(ShadowPos)) continue;

		FrustumID = i;
		ShadowPos.xy = (ShadowPos.xy / ShadowPos.w)*0.5f + 0.5f;
		ShadowPos.y = 1.0f - ShadowPos.y;

		if (ShadowPos.x <= 0.05f || ShadowPos.x >= 0.95f || ShadowPos.y <= 0.05f || ShadowPos.y >= 0.95f) {
			continue;
		}

		float2 ShadowCoord = ShadowPos.xy;
		ShadowCoord.y /= float(NUM_SHADOW_CASCADES);
		ShadowCoord.y += float(i / float(NUM_SHADOW_CASCADES));
		//ShadowPos.z += +1e-6;
		float shadow = 0.0f;

		static float kernel[25] = {

			0.003765f,	0.015019f,	0.023792f,	0.015019f,	0.003765f,
			0.015019f,	0.059912f,	0.094907f,	0.059912f,	0.015019f,
			0.023792f,	0.094907f,	0.150342f,	0.094907f,	0.023792f,
			0.015019f,	0.059912f,	0.094907f,	0.059912f,	0.015019f,
			0.003765f,	0.015019f,	0.023792f,	0.015019f,	0.003765f

		};

		const int step = 1;
		const int down = -2;
		const int up = 2;
		const float cascadeSize = 512.0f;

		float stepSize = SHADOW_BLURS[i];

		result = 0.0f;
		[unroll]
		for (int x = down; x <= up; x += step) {
			[unroll]
			for (int y = down; y <= up; y += step) {
				shadow += ShadowMask0.Sample(SecondarySampler, (
					(ShadowCoord.xy + float2(x * stepSize, float(y * stepSize / float(NUM_SHADOW_CASCADES))) / cascadeSize) * float2(globalQuality, globalQuality)
				)).r * kernel[(y + 2) * 5 + x + 2];
			}
		}

		result = CalcVarianceShadow(shadow, ShadowPos.z, VARIANCE_SHADOW[i]);
		break;
	}
	return clamp(result, 0.0f, 1.0f);
}

#endif

#ifdef INCLUDE_LUT
float4 Lookup(float4 textureColor) {
	textureColor = clamp(textureColor, 0.0f, 1.0f);

	float2 bg = clamp(float2(textureColor.b / 8.0f, textureColor.g / 8.0f), 0.001953125f, 0.123046875f);
	uint red = uint(floor(textureColor.r * 64.0f));
	uint col = min(7, red % 8);
	uint row = min(7, red / 8) % 8;
	float2 pos = float2(col / 8.0f, row / 8.0f) + bg;

	return LUT.Sample(MainSampler, float2(pos.x, pos.y));
}
#endif

#ifdef INCLUDE_SSAO

float3 Rand3(float2 uv) {
	return float3(
		frac(abs(sin(12.8 * uv.x * uv.y * time))),
		frac(abs(cos(uv.x + 9.3 * uv.y * time))),
		frac(abs(sin(7.81 * uv.x * uv.y + time))));
}
float SSAO(float2 uv) {
	return 1.0f;
}

#endif