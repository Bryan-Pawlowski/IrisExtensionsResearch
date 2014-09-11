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
	float4 position : POSITION;
};

VOut VShader(float4 position : POSITION, float4 normal : NORMAL)
{
	VOut output;
	output.svposition = mul(final, position);
	output.position = mul(final, position);


	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm, lightvec));
	output.color += lightcol * diffusebrightness;

	return output;
}
RWTexture2D<uint>  pixelTouched			: register (u1);
RWTexture2D<float> pixDepth				: register (u2);
RWTexture2D<float4> prevCol				: register (u3);
float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION) : SV_TARGET
{
	uint2 pixelAddr = svposition.xy;
	uint2 dim;
	bool touched;
	float total;
	
	IntelExt_Init();

	IntelExt_BeginPixelShaderOrderingOnUAV( 2 );

	float depth = pixDepth[pixelAddr];

	if (depth == 0)
	{
		depth = position.z;
	}
	else {
		total = abs(position.z - depth);
		color.g = total/2;
		depth = position.z;
	}
	pixDepth[pixelAddr] = depth;
	return color;
}