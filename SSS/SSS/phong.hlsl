cbuffer ConstantBuffer
{
	float4x4 final;		  // the modelViewProjection matrix
	float4x4 rotation;    // the rotation matrix
	float4x4 modelView;   // the modelView Matrix
	float4 lightvec;      // the light's vector
	float4 lightcol;      // the light's color
	float4 ambientcol;    // the ambient light's color
	float4 camera;
	uint samp;
}

struct VOut
{
	float4 svposition;
	float4 color;
	float4 normal;
	float4 light;
	float4 eye;
};

//we need normal, light and eye coordinates.

VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
{
	VOut output;

	output.svposition = position;
	output.color = color;
	output.normal = normal;
//	output.light = ;
//	output.eye = ;

	return output;
}