#include "./IGFXExtensions/IntelExtensions.hlsl"

cbuffer ConstantBuffer
{
	float4x4 final;		  // the modelViewProjection matrix
	float4x4 rotation;    // the rotation matrix
	float4x4 modelView;   // the modelView Matrix
	float4 lightvec;      // the light's vector
	float4 lightcol;      // the light's color
	float4 ambientcol;    // the ambient light's color
	float4 camera;		  // the camera position, passed in.
	float4 camMat;
}

struct VOut
{
	float4 svposition : SV_POSITION;
	float4 color : COLOR;
	float4 position : POSITION;
	float2 UVs : UV;
	float4 normal : NORMAL;
	float4 camera : CAMERA;
};

VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : UV)
{
	VOut output;
	output.svposition = mul(final, position);
	output.position = mul(final, position);
	output.camera = mul(camMat, camera);


	//we need some knowledge of the camera.

	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm1 = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm1, lightvec));
	float4 norm = normalize(mul(final, normal));
	//if (texCoord.x >= 0.5f) output.color.b = 1.0;
	output.color += lightcol * diffusebrightness;

	output.UVs = texCoord;

	output.normal = norm;

	return output;
}

globallycoherent RWTexture2D<float> Shallow				: register (u1); //keep track of shallowest point in XY position relative to screen
globallycoherent RWTexture2D<float> uvDepth				: register (u2); //keep track of depth of that UV coordinate.
globallycoherent RWTexture2D<uint>  fromLightX			: register (u3); //X pixel coordinates from the light.
globallycoherent RWTexture2D<uint>  fromLightY			: register (u4); //Y pixel coordinates from the light.

float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV, float4 norm : NORMAL, float4 camera : CAMERA) : SV_TARGET 
{
	uint2 pixelAddr = svposition.xy;
	uint2 uv;
	uv.x = int(720.f * UVs.x);
	uv.y = int(720.f * UVs.y);

	float pos = distance(position, camera);

		IntelExt_Init();

		IntelExt_BeginPixelShaderOrderingOnUAV(2);
		float currDepth = Shallow[pixelAddr];

		//we need the depth at that UV spot on the model.
		uvDepth[uv] = pos;

		//we need the shallowest depth (one closest to the light).
		if (currDepth == 0) currDepth = pos;
		else if (currDepth >= pos) currDepth = pos;
		
		Shallow[pixelAddr] = currDepth;
		
	
		fromLightX[uv] = pixelAddr.x;
		fromLightY[uv] = pixelAddr.y;

	return color;
}

float4 PShader2(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV, float4 norm : NORMAL) : SV_TARGET
{
	uint2 uv;
	uv.x = int(720.f * UVs.x);
	uv.y = int(720.f * UVs.y);
	float tol = .009;


	uint2 lightCoords;

	lightCoords.x = fromLightX[uv];
	lightCoords.y = fromLightY[uv];

	float mdepth = uvDepth[uv];
	float shallow = Shallow[lightCoords];

	if (!((shallow >= (mdepth - tol)) && (shallow <= (mdepth + tol))))
	{
		color.a *= (1 - (mdepth - shallow));
	}

	return color;
}