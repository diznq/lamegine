#include "Constants.hlsli"
#define INCLUDE_SHADOWS
#define INCLUDE_SSAO

static float VARIANCE_SHADOW[] = { 6.0f, 12.0f, 18.0f };
static float SHADOW_BLURS[] = { 0.5f, 0.25f, 0.125f };
#include "FxLibrary.hlsli"
#include "AreaLights.hlsli"

#define PI 3.14159265359
#define RECIPROCAL_PI 0.31830988618
#define RECIPROCAL_PI2 0.15915494

#include "PBR.hlsli"

float4 SampleReflection(float3 dir, float rougness)
{
	dir = normalize(dir);
	half theta = acos(dir.z); // +z is up
	half phi = atan2(-dir.y, -dir.x) + PI;
	half2 uv = half2(phi, theta) * half2(0.1591549, 0.31830988618);
	return ReflMap.SampleLevel(MainSampler, uv, rougness * 8);
}

float4 SampleIrradiance(float3 dir)
{
	dir = normalize(dir);
	half theta = acos(dir.z); // +z is up
	half phi = atan2(-dir.y, -dir.x) + PI;
	half2 uv = half2(phi, theta) * half2(0.1591549, 0.31830988618);
	return Irradiance.Sample(MainSampler, uv);
}

float3 LocalCorrect(float3 origVec, float3 bboxMin, float3 bboxMax, float3 vertexPos, float3 cubemapPos) {
	float3 invOrigVec = float3(1.0f, 1.0f, 1.0f) / (origVec);
	float3 intersecAtMaxPlane = (bboxMax - vertexPos) * invOrigVec;
	float3 intersecAtMinPlane = (bboxMin - vertexPos) * invOrigVec;
	float3 largestIntersec = max(intersecAtMaxPlane, intersecAtMinPlane);
	float Distance = min(min(largestIntersec.x, largestIntersec.y), largestIntersec.z);
	float3 IntersectPositionWS = vertexPos + origVec * Distance;
	float3 localCorrectedVec = IntersectPositionWS - cubemapPos;
	//localCorrectedVec.z /=  (bboxMax.z - bboxMin.z);
	return (localCorrectedVec);
}

//#define USE_SSAO

float4 CalcFinalColor(float4 wsPosition, float4 wsNormal, float2 uv, in float3 reflDir, out float2 LightingInfo, out float3 Specular) {
	Specular.rgb = float3(0.0f, 0.0f, 0.0f);
	float FogDensity = fogSettings.a;
	float Distance = distance(wsPosition.xyz, cameraPos.xyz);
	float FogRatio = 1.0f - clamp(exp(-pow(FogDensity*Distance, 2.0)), 0.0f, 1.0f);
	const float ShadowPower = 0.2f;
	float ShadowMin = shadowStrength;
	uint FrustumID = 0;
	float ShadowRatio = clamp(pow(CalcShadow(wsPosition.xyz, uv, FrustumID), 4), ShadowMin, 1.0f);

	float4 PBR = Misc.Sample(MainSampler, uv);
	#ifdef FORWARD
	PBR = PBR * instances[INSTANCE_INDEX].pbrInfo;
	#endif

	float specularRatio = PBR.SPECULAR;

	float3 SunColor = sunColor.rgb;
	float3 SunLight = CalcDirLightColor(SunColor, 1.0f, wsPosition.xyz, wsNormal.xyz, sunDir.xyz, Specular, specularRatio, ShadowRatio);
	//if (uv.x < 0.5f) SunLight = SunColor;

#ifdef MSAA
	float4 Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float part = 1.0f / 8;
	[unroll] for (int k = 0; k < 8; k++)
		Color += Albedo.Load(uint2(uv * resolution), k) * part;
#else
	float4 Color = Albedo.Sample(MainSampler, uv);// *SampleIrradiance(wsNormal.xyz);
#endif

	float3 albedo = Color.rgb;
	Color.rgb *= 1.0f - pow(min(0.5f, PBR.SPECULAR * PBR.ROUGHNESS), 4);

	float3 PointLight = float3(0.0f, 0.0f, 0.0f);

	[unroll]
	for (uint i = 0; i < NUM_LIGHTS; i++) {
		PointLight += CalcPointLightColor(lights[i].color.rgb, lights[i].intensity, wsPosition.xyz, wsNormal.xyz, lights[i].position, Specular, specularRatio, 1.0f);
	}

	float PointLights = (PointLight.r + PointLight.g + PointLight.b) / 3.0f;
	float AO = PBR.AOVAL;

	//if (PBR.SPECULAR == PBR.ROUGHNESS && PBR.ROUGHNESS == PBR.AOVAL) AO = 1.0f;

	float Shadowness = saturate(
		(PointLights + clamp(
			ShadowRatio * AO * (SunLight.r*0.2 + SunLight.g*0.7 + SunLight.b*0.1),
			ShadowMin,
			1.0f
		)
	));
	float3 LightColor = float3(1.0f, 1.0f, 1.0f) + PointLight;
	
	float3 CubemapReflectionDir = reflDir;
	if (cubemapPos.w > 0.0f)
		CubemapReflectionDir = LocalCorrect(CubemapReflectionDir.xyz, cubemapBboxA.xyz, cubemapBboxB.xyz, wsPosition.xyz, cubemapPos.xyz);

#if 1
#ifdef FORWARD
	float3 Dir = normalize(cameraPos.xyz - wsPosition.xyz);
	float IncidenceAngle = dot(Dir, wsNormal.xyz);
	float3 ReflColor = SampleReflection(CubemapReflectionDir, PBR.ROUGHNESS + IncidenceAngle * 0.33f).rgb * pow(PBR.SPECULAR, 2) * saturate(1.0f - abs(IncidenceAngle)) * AO;

	Specular.rgb += ReflColor;
#else
	//float3 ReflColor = SampleReflection(CubemapReflectionDir, PBR.ROUGHNESS).rgb * (PBR.SPECULAR) * AO;
#endif
#endif
	Color.rgb *= LightColor;

	LightingInfo.r = Shadowness;
	LightingInfo.g = FogRatio;

	return Color;
}

float4 PostFxColor(VS_OUTPUT input, out float2 lightingInfo, out float3 Specular) {
	Specular.rgb = float3(0.0f, 0.0f, 0.0f);
	lightingInfo.rg = float2(0.0f, 0.0f);
	float2 uv = input.Texcoord;
#ifdef FORWARD
	uv *= instances[INSTANCE_INDEX].txScale;
	float4 vsDepth = input.Position.z;
#else
	float4 vsDepth = Depth.Sample(MainSampler, uv);
#endif
	if (vsDepth.r < 1.0f) {
#ifdef FORWARD
		float4 wsPosition = float4(input.WSPosition, 1.0f);
		float4 wsNormal = GetBumpNormal(input.Normal, input.Tangent, uv, Normal.Sample(MainSampler, uv));
#else
		float4 wsPosition = Geometry.Sample(MainSampler, uv);
		float4 wsNormal = Normal.Sample(MainSampler, uv);
#endif
		Ray Reflection;
		Reflection.origin = wsPosition.xyz;
		Reflection.dir = reflect(wsPosition.xyz - cameraPos.xyz, wsNormal.xyz);//normalize(reflect(normalize(wsPosition.xyz - cameraPos.xyz), wsNormal.xyz));

		float4 Color = CalcFinalColor(wsPosition, wsNormal, uv, Reflection.dir, lightingInfo, Specular);

		float3 RadiosityPos = (wsPosition.xyz / radiosityRes.xyz + 1.0f) / 2.0f;
		float3 RadiosityColor = Radiosity.SampleLevel(MainSampler, RadiosityPos.xy, 0).rgb;
		Color.rgb *= pow(abs(lerp(float3(1.0f, 1.0f, 1.0f), RadiosityColor, radiositySettings.r)), radiositySettings.g);
		Color.rgb += pow(abs(RadiosityColor), radiositySettings.w) * radiositySettings.b;
		return Color;
	} else {
#ifdef FORWARD
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
#else
		lightingInfo.r = 1.0f;
		lightingInfo.g = 0.0f;
		float4 sunPos = TranslateRay(cameraPos.xyz + sunDir.xyz * 1000);
		float2 nrm = normalize(resolution);
		float dst = distance(sunPos.xy * nrm, uv.xy * nrm);
		Specular.rgb = float3(0.0f, 0.0f, 0.0f);

		const float sunSize = 0.0125f;
		if (sunPos.w < 0.0f && dst < sunSize) {
			float mult = 1.0f - (sunSize - dst) / sunSize;
			//lightingInfo.g = 0.95f;
			return float4(sunColor.rgb * 20.0f, 1.0f);
		}
		return float4(skyColor.rgb, 1.0f);
#endif
	}
}

#ifdef USE_PBR
PBR_OUTPUT PBRColor(PBR_INPUT input) {
	return BRDF(input);
}
#endif