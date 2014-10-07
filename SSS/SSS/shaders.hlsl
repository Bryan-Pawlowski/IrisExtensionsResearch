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
	float4 normal : NORMAL;
};

VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : UV)
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

	output.UVs = texCoord;

	output.normal = norm;

	return output;
}
RWTexture2D<uint>  pixelTouched			: register (u1);
RWTexture2D<float> pixDepth				: register (u2);
RWTexture2D<float> modelDepth			: register (u3);


float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV, float4 norm : NORMAL) : SV_TARGET {
	uint2 pixelAddr = svposition.xy;
	bool touched;
	float total;
	uint2 uv;
	uv.x = int(256.f * UVs.x);
	uv.y = int(256.f * UVs.y);

	float pos = position.z;
	float4 n = normalize(norm);

		IntelExt_Init();

		IntelExt_BeginPixelShaderOrderingOnUAV(2);

		float depth = pixDepth[pixelAddr];
		float mdepth = modelDepth[uv];

		uint touch = pixelTouched[pixelAddr];
		
		mdepth = position.z;

		pixDepth[pixelAddr] = depth;
		modelDepth[uv] = mdepth;
		pixelTouched[pixelAddr] = touch;
		
	return color;
}

float4 PShader2(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV, float4 norm : NORMAL) : SV_TARGET{
	uint2 uv;

	uv.x = int(256.f * UVs.x);
	uv.y = int(256.f * UVs.y);

	float mdepth = modelDepth[uv];


	if (mdepth < 0.0f) discard;

	return color;
}