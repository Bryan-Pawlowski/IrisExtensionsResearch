TextureCube SkyMap;
SamplerState ObjSamplerState;


cbuffer cbPerObject
{
	float4x4 WVP;
	float4x4 World;
	float4x4 Rotation;
	float4 LightVector;
	float4 LightColor;
	float4 AmbientColor;
};

struct SKYMAP_VS_OUTPUT //output struct for skymap v shader
{
	float4 Pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

SKYMAP_VS_OUTPUT SKYMAP_VS(float4 inPos: POSITION, float3 normal : NORMAL, float2 inTexCoord : TEXCOORD)
{
	SKYMAP_VS_OUTPUT output = (SKYMAP_VS_OUTPUT)0;
	inPos.w = 1.0f;
	output.Pos = mul( inPos, WVP).xyww;

	//output.Pos = mul(float4(inPos, 1.0f), WVP);

	output.texCoord = inPos;

	return output;
}

float4 SKYMAP_PS(SKYMAP_VS_OUTPUT input) : SV_TARGET
{

	return SkyMap.Sample(ObjSamplerState, input.texCoord);

	//return float4(0.0, 1.0, 0.0, 1.0);
}