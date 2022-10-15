struct PS_INPUT
{
	float4 Position		: SV_POSITION;
	float2 Texcoord		: TEXCOORD0;
	float3 Normal		: TEXCOORD1;
	float3 Tangent		: TEXCOORD2;
	float3 WSPosition	: TEXCOORD3;
};

struct PS_OUTPUT {
	//float Color		: SV_TARGET;
};

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT Output;

	//float depth = input.Position.z;
	//Output.Color = depth;

	return Output;
}
