#include "./IGFXExtensions/IntelExtensions.hlsl"
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
	float2 UVs : UV;
};

VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : UV)
{
	VOut output;
	output.svposition = mul(final, position);
	output.position = mul(final, position);


	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm, lightvec));
	//if (texCoord.x >= 0.5f) output.color.b = 1.0;
	output.color += lightcol * diffusebrightness;

	output.UVs = texCoord;

	return output;
}
RWTexture2D<uint>  pixelTouched			: register (u1);
RWTexture2D<float> pixDepth				: register (u2);
RWTexture2D<float> modelDepth			: register (u3);
float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV) : SV_TARGET //this pshader is for the rendering from the light source.
{
	uint2 pixelAddr = svposition.xy;
	uint2 dim;
	bool touched;
	float total;
	uint2 uv;
	uv.x = int(1024.f * UVs.x);
	uv.y = int(1024.f * UVs.y);

	float pos = position.z;

	//color.b = (float)uv.x / 1024.f;
	//color.r = (float)uv.y / 1024.f;
	
	IntelExt_Init();

	IntelExt_BeginPixelShaderOrderingOnUAV( 2 );

	float depth = pixDepth[pixelAddr];
	float mdepth = modelDepth[uv];


	if (depth == 0) {
		depth = position.z;
		mdepth = 0;
	}
	else{
		if (pos > depth) total = pos - depth;
		else total = depth - pos;

		color.g += total / 10;
		mdepth = total;
	}

	pixDepth[pixelAddr] = depth;
	modelDepth[uv] = mdepth;

	return color;
}