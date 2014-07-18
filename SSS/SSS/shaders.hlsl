#include "./IGFXExtensions/IntelExtensions.hlsl"
#include "subSurface.hlsl"

cbuffer ConstantBuffer
{
	float4x4 final;		  // the modelViewProjection matrix
	float4x4 rotation;    // the rotation matrix
	float4x4 modelView;   // the modelView Matrix
	float4 lightvec;      // the light's vector
	float4 lightcol;      // the light's color
	float4 ambientcol;    // the ambient light's color
}

struct VOut
{
	float4 svposition : SV_POSITION;
	float4 color : COLOR;
	float4 normal : NORMAL0;
	float4 lightV : NORMAL1;
	float4 eyeV : NORMAL2;
	float4 position : POSITION;
};

VOut VShader(float4 position : POSITION, float4 normal : NORMAL)
{
	VOut output;

	float4 ECPosition = mul(modelView, position);
		//vector from point to light
		float3 ECP;
		float3 lightVect = lightvec.xyz - ECPosition.xyz;
		//vector from point to eye position
		float3 eyeVect = float3(0.0, 0.0, 0.0) - ECPosition.xyz;

		output.lightV = float4(lightVect, 1.0);
	output.eyeV = float4(eyeVect, 1.0);

	output.svposition = mul(final, position);
	output.position = mul(final, position);


	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm, lightvec));
	output.color += lightcol * diffusebrightness;

	output.normal = norm;

	return output;
}

float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 normal : NORMAL0, float4 lightV : NORMAL1, float4 eyeV : NORMAL2, float4 position : POSITION) : SV_TARGET
{
	uint2 pixelAddr = svposition.xy;
	uint2 dim;
	bool touched;

	depthUAV.GetDimensions(dim[0], dim[1]);

	IntelExt_Init();

	IntelExt_BeginPixelShaderOrdering( );
	if (dim.y == 0)
	{
		color.r = 1.0;
	}


	//if(position.z > 5.5f) color.a = 1.0;


	return color;
}