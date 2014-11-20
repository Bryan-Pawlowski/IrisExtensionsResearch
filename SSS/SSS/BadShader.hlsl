#include "./IGFXExtensions/IntelExtensions.hlsl"

#define SCREEN_WIDTH	800.f
#define SCREEN_HEIGHT	600.f

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
	float4 svposition : SV_POSITION;
	float4 color : COLOR;
	float4 position : POSITION;
	float2 UVs : TEXCOORD;
	float4 normal : NORMAL;
	float4 camera : CAMERA;
	uint samp : SAMPLE;
};


VOut VShader2(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
{
	VOut output;
	float2 movedCoords = texCoord * 2;
		movedCoords.x -= 1;
	movedCoords.y -= 1;
	float4 svPos = float4(movedCoords, 0.0, 1.0);
		output.svposition = svPos;
	output.position = mul(final, position);


	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm1 = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm1, lightvec));
	float4 norm = normalize(mul(final, normal));
		//if (texCoord.x >= 0.5f) output.color.b = 1.0;
		output.color += lightcol * diffusebrightness;

	output.UVs.x = texCoord.x * SCREEN_WIDTH;
	output.UVs.y = texCoord.y * SCREEN_HEIGHT;

	output.normal = norm;

	output.camera = mul(modelView, camera);

	output.samp = samp;


	return output;
}

RWTexture2D<float> uvDepth				: register (u1); //keep track of depth of that UV coordinate.
RWTexture2D<float> Shallow				: register (u2); //keep track of shallowest point in XY position relative to screen
RWTexture2D<float>  fromLightX			: register (u3); //X pixel coordinates from the light.
RWTexture2D<float>  fromLightY			: register (u4); //Y pixel coordinates from the light.


float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : TEXCOORD, float4 norm : NORMAL, float4 camera : CAMERA) : SV_TARGET
{
	uint2 pixelAddr = position.xy;
	float2 svPos = position.xy;
	uint2 uv = UVs;

	svPos += 1;
	svPos = svPos / 2;

	svPos.x = svPos.x * SCREEN_WIDTH;
	svPos.y = svPos.y * SCREEN_HEIGHT;

	//we need knowledge of the screen size, but we are making progress, here.

	float pos = distance(position, camera);
	float mdepth = distance(position, camera);

	fromLightX[uv] = pixelAddr.x;
	fromLightY[uv] = pixelAddr.y;
	uvDepth[uv] = mdepth;

	return color;
}
