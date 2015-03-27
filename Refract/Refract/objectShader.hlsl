cbuffer cbPerObject
{
	float4x4 WVP;
	float4x4 World;
};



struct VOUT
{
	float4 Pos : SV_POSITION;
	float4 Normal : NORMAL;
	float2 texCoord : TEXCOORD;
};


struct VOUT VShader( float4 pos : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD )
{
	VOUT output;

	output.Pos = pos;
	output.Normal = normal;
	output.texCoord = texCoord;

	return output;
}

float4 PShader(float4 svpos : SV_POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
{

	return float4(0.0, 1.0, 0.0, 1.0);
}