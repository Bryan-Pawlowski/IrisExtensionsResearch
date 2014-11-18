#include "./IGFXExtensions/IntelExtensions.hlsl"

#define TEXSIZE 256.f
#define SCREEN_WIDTH	1920.f
#define SCREEN_HEIGHT	1080.f

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

VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
{
	VOut output;
	output.svposition = mul(final, position);
	output.position = mul(final, position);


	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm1 = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm1, lightvec));
	float4 norm = normalize(mul(final, normal));
	//if (texCoord.x >= 0.5f) output.color.b = 1.0;
	output.color += lightcol * diffusebrightness;

	output.UVs.x = texCoord.x * TEXSIZE;
	output.UVs.y = texCoord.y * TEXSIZE;

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
	uint2 pixelAddr = svposition.xy;
	uint2 uv;
	uv.x = (uint)UVs.x;
	uv.y = (uint)UVs.y;

	float pos = distance(position, camera);
	float mdepth = distance(position, camera);
	IntelExt_Init();

	IntelExt_BeginPixelShaderOrderingOnUAV(2);



	fromLightX[uv] = (float)pixelAddr.x;
	fromLightY[uv] = (float)pixelAddr.y;
	uvDepth[uv] = mdepth;


	if (pos < Shallow[pixelAddr]) Shallow[pixelAddr] = pos;


	color.g += ((mdepth - Shallow[pixelAddr])/5);

	return color;
}





float4 PShader2(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV, float4 norm : NORMAL, float4 camera : CAMERA, uint samp : SAMPLE) : SV_TARGET
{
	uint2 uv;
	uv.x = (uint)UVs.x;
	uv.y = (uint)UVs.y;


	float arc = dot(norm, camera);
	float rad = acos(arc);
	float deg = degrees(rad);

	float tol = 25.05;

	if ((deg <= 90.f - tol) && (deg >= 90.f + tol)) return color;

	float2 lightCoords;

	lightCoords.x = fromLightX[uv];
	lightCoords.y = fromLightY[uv];

	uint2 ilc;

	ilc.x = (int)lightCoords.x;
	ilc.y = (int)lightCoords.y;

	if ((lightCoords.x == -1) && (lightCoords.y == -1)){
		color.r = 1.0;
	}

	float mdepth = uvDepth[uv];

	float shallow = Shallow[ilc];

	if (mdepth == 100.f) color.b = 1.0f;
		
	if(((shallow != 100.f) && (mdepth != 100.f)) && (mdepth > shallow)) color.rgb *= 1 - ((mdepth - shallow)*.25);
		return color;
}

