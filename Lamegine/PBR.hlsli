#ifndef GLSL_SUPPORT
#define GLSL_SUPPORT
#define vec3 float3
#define vec4 float4
#define vec2 float2
#endif

#define SPECULAR r
#define METALLIC r
#define ROUGHNESS g
#define AOVAL b
#define DISPLACEMENT b
#define BUMP a

struct PBR_OUTPUT {
	float3 color;
};

struct PBR_INPUT {
	float2 uv;
	float3 albedo;
	float4 ddna;
	float4 pbr;
	float3 position;
	float3 reflection;
	float3 dir;
	float cosine;
	float3 irradiance;
	float invShadow;
	float depth;
	float fog;
};

static const float3 Fdielectric = 0.04;
static const float Epsilon = 0.00001;

#ifdef USE_PBR
uint QuerySpecularTextureLevels()
{
	uint width, height, levels;
	ReflMap.GetDimensions(0, width, height, levels);
	return levels;
}

float4 SampleReflectionPBR(float3 dir, float rougness)
{
	dir = normalize(dir);
	uint levels = QuerySpecularTextureLevels();
	half theta = acos(dir.z); // +z is up
	half phi = atan2(-dir.y, -dir.x) + PI;
	half2 uv = half2(phi, theta) * half2(0.1591549, 0.31830988618);
	return ReflMap.SampleLevel(MainSampler, uv, rougness * levels);
}


float4 SampleIrradiancePBR(float3 dir)
{
	dir = normalize(dir);
	half theta = acos(dir.z); // +z is up
	half phi = atan2(-dir.y, -dir.x) + PI;
	half2 uv = half2(phi, theta) * half2(0.1591549, 0.31830988618);
	return Irradiance.Sample(MainSampler, uv);
}

float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
float3 fresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

PBR_OUTPUT BRDF(PBR_INPUT input) {
	PBR_OUTPUT o;
	o.color = float3(0.0f, 0.0f, 0.0f);

	float4 ndc = float4(float2(2.0f * input.uv.x - 1.0f, 2.0f * (1.0f - input.uv.y) - 1.0f), input.depth, 1.0f);
	float3 viewDir = normalize(mul(mul(ndc, invProj), invView).xyz);

	if (input.depth < 1.0f) {
		float3 color = float3(0.0f, 0.0f, 0.0f);
		float3 specular = float3(0.0f, 0.0f, 0.0f);
		float3 diffuse = input.albedo * input.irradiance * input.pbr.AOVAL;
		float metallic = input.pbr.METALLIC;
		float roughness = input.pbr.ROUGHNESS;
		float3 N = normalize(input.ddna.xyz);
		float3 V = normalize(input.dir);
		float3 F0 = lerp(Fdielectric, input.albedo, metallic);
		float3 Lr = input.reflection;

		[unroll]
		for (int i = 0; i < NUM_LIGHTS; i++) {
			float dst = distance(input.position, lights[i].position);
			float3 L = normalize(lights[i].position.xyz - input.position.xyz);
			float3 H = normalize(V + L);
			float3 LR = lights[i].color.xyz * lights[i].intensity / pow(dst, 2);
			float NdotL = max(0.0, dot(N, L));
			float NdotH = max(0.0, dot(N, H));
			float HdotV = max(0.0, dot(H, V));

			float3 F = fresnelSchlick(F0, HdotV);
			float D = ndfGGX(NdotH, roughness);
			float G = gaSchlickGGX(NdotL, NdotH, roughness);

			float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

			float3 nominator = D * G * F;
			float denominator = max(Epsilon, 4.0 * NdotL * NdotH); // 0.001 to prevent divide by zero.
			float3 brdf = nominator / denominator;
			float3 kS = F;
			float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1 - metallic);

			specular += brdf * LR * NdotL;
			color += kd * LR * input.albedo * NdotL;

			float3 diffuseBRDF = kd * input.albedo;
			float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * NdotL * NdotH);

			//o.color += (diffuseBRDF) * LR * NdotL;
			//o.specular += specularBRDF * LR * NdotL;
		}

		float3 irradiance = input.irradiance;
		float3 L = normalize(-sunDir.xyz);
		float3 H = normalize(V + L);

		float NdotL = max(0.0, dot(N, L));
		float NdotH = max(0.0, dot(N, H));
		float HdotV = max(0.0, dot(H, V));
		
		float shadowing = min(max(shadowStrength, NdotL), input.invShadow);

		float3 F = fresnelSchlick(F0, HdotV);

		float3 kd = lerp(1.0 - F, 0.0, 0);
		float3 diffuseIBL = kd * input.albedo * irradiance;

		// Sample pre-filtered specular reflection environment at correct mipmap level.
		float3 specularIrradiance = SampleReflectionPBR(Lr, roughness).rgb;

		// Split-sum approximation factors for Cook-Torrance specular BRDF.
		float2 specularBRDF = SpecularBRDFLUT.Sample(MainSampler, float2(HdotV, roughness)).rg;

		// Total specular IBL contribution.
		float Cos = max(input.invShadow, NdotL * input.invShadow);
		float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

		color += diffuseIBL;
		color *= shadowing;
		specular += specularIBL;
		o.color = color + specular;

		o.color = lerp(o.color, SampleIrradiancePBR(viewDir), input.fog);
	} else {
		o.color = SampleReflectionPBR(viewDir, 0.0f).rgb;
	}

	return o;
}
#endif