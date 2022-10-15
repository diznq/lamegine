#include "Constants.hlsli"

struct VS_OUTPUT
{
    float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
	uint InstanceID		: Output;
};

 
VS_OUTPUT main(VS_INPUT input, uint iid : SV_InstanceID)
{
	const uint INSTANCE_INDEX = iid + instance_start.x;
    VS_OUTPUT Output;

    float4 pos = float4(input.pos, 1.0f);

	float4 totalLocalPos = float4(0.0, 0.0f, 0.0f, 0.0f);
	float4 totalNormal = float4(0.0, 0.0f, 0.0f, 0.0f);

	if (input.jointIds[0] != -1) { 
		for (int i = 0; i < 3; i++) {
			matrix jointTransform = jointTransforms[input.jointIds[i]];
			float4 posePosition = mul(float4(input.pos, 1.0f), jointTransform);
			totalLocalPos += posePosition * input.weights[i];
			float4 worldNormal = mul(float4(input.normal, 0.0f), jointTransform);
			totalNormal += worldNormal * input.weights[i];
		}
	} else {
		totalLocalPos = float4(input.pos, 1.0f);
		totalNormal = float4(input.normal, 0.0f);
	}

	const InstanceData instance = instances[INSTANCE_INDEX];

	pos = mul(totalLocalPos, instance.model);

	float4 realPos = pos;

    pos = mul(pos, view);
    pos = mul(pos, proj);

    Output.Position = pos;
	Output.Texcoord = input.texcoord + float2(instance.pbrExtra.z * time, instance.pbrExtra.w * time);
	Output.Normal = mul(totalNormal.xyz, (float3x3)instance.model);
	Output.Tangent = mul(input.tangent, (float3x3)instance.model);
	Output.WSPosition = realPos.xyz;
	Output.InstanceID = INSTANCE_INDEX;

	//float dst = distance(cameraPos, realPos);
	//const float MaxTess = 8.0f;
	//Output.TessFactor = 1.0f + (1.0f - saturate(dst / MaxTess))*MaxTess;

    return Output;
}
