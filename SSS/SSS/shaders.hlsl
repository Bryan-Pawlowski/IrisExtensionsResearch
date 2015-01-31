#include "./IGFXExtensions/IntelExtensions.hlsl"

#define SCREEN_WIDTH	1920.f
#define SCREEN_HEIGHT	1080.f

#define PIXSYNC_OFF		1024
#define DIFFUSE_WRAP	512
#define PHONG_RENDER	256
#define INVERT			1

#define SHININESS		1.0

cbuffer ConstantBuffer
{
	float4x4 final;		  // the modelViewProjection matrix
	float4x4 rotation;    // the rotation matrix
	float4x4 modelView;   // the modelView Matrix
	float4 lightvec;      // the light's vector
	float4 lightcol;      // the light's color
	float4 ambientcol;    // the ambient light's color
	float4 lightPos;
	uint mode;
}

struct VOut
{
	float4 svposition : SV_POSITION;
	float4 color : COLOR;
	float4 position : POSITIONT;
	float2 UVs : TEXCOORD;
	float4 normal : NORMAL;
	float4 camera : CAMERA;
	float3 lightVec : NORMAL1;
	float4 lightCol : COLOR1;
	uint mode : MODE;
	float4 rotNorm : NORMAL2;
	float3 eyeVec : NORMAL3;
};



RWTexture2D<float> uvDepth				: register (u1); //keep track of depth of that UV coordinate.
RWTexture2D<float> Shallow				: register (u2); //keep track of shallowest point in XY position relative to screen
RWTexture2D<float>  fromLightX			: register (u3); //X pixel coordinates from the light.
RWTexture2D<float>  fromLightY			: register (u4); //Y pixel coordinates from the light.

VOut VShader(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
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

	//output.color += lightcol * diffusebrightness;

	output.UVs.x = texCoord.x * SCREEN_WIDTH;
	output.UVs.y = texCoord.y * SCREEN_HEIGHT;

	output.normal = norm;

	output.camera = mul(final, lightPos);

	output.mode = mode;


	return output;
}

VOut VShader2(float4 position : POSITION, float4 normal : NORMAL, float2 texCoord : TEXCOORD)
{
	VOut output;
	
		
	output.svposition = mul(final, position);
	output.position = mul(final, position);

	float4 ECposition = mul(modelView, position);
		float3 eyeLightPosition = lightPos.xyz;

		float3 lightVec = eyeLightPosition - ECposition.xyz;
		float3 eyeVec = float3(0, 0, 0) - ECposition.xyz;

		output.lightVec = normalize(lightVec);
	output.eyeVec = normalize(eyeVec);
	// set the ambient light
	output.color = ambientcol;

	// calculate the diffuse light and add it to the ambient light
	float4 norm1 = normalize(mul(rotation, normal));
		float diffusebrightness = saturate(dot(norm1, lightvec));
	float4 norm = normalize(mul(final, normal));

	//output.color += lightcol * diffusebrightness;

	output.lightCol = lightcol;

	output.lightVec = lightvec;

	output.UVs.x = texCoord.x * SCREEN_WIDTH;
	output.UVs.y = texCoord.y * SCREEN_HEIGHT;

	output.normal = normal;

	output.camera = mul(final, lightPos);

	output.mode = mode;

	output.rotNorm = norm1;


	return output;
}

float4 PShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITIONT, float2 UVs : TEXCOORD, float4 norm : NORMAL, float4 camera : CAMERA) : SV_TARGET 
{
	float2 svPos = position.xy;
	float mdepth = distance(position, camera);
	uint2 uv = UVs;

	svPos.y = 0 - svPos.y;
	svPos += 1;
	svPos = svPos / 2;

	float2 newPos;

	newPos.x = svPos.x * SCREEN_WIDTH;
	newPos.y = svPos.y * SCREEN_HEIGHT;

	fromLightX[uv] = newPos.x;
	fromLightY[uv] = newPos.y;
	uvDepth[uv] = mdepth;

	return color;
}

float4 POShader(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITIONT, float2 UVs : TEXCOORD, float4 norm : NORMAL, float4 camera : CAMERA) : SV_TARGET
{
	uint2 pixelAddr = svposition.xy;

	float pos = distance(position, camera);

	IntelExt_Init();
	IntelExt_BeginPixelShaderOrdering();

	if (pos < Shallow[pixelAddr])
	{
		Shallow[pixelAddr] = pos;
	}

	return color;
	
}




float4 PShader2(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITIONT, float2 UVs : UV, float4 norm : NORMAL, float4 camera : CAMERA,
	float3 lightVec : NORMAL1, float4 lightCol : COLOR1, uint mode : MODE, float4 rotNorm : NORMAL2, float3 eyeVec : NORMAL3) : SV_TARGET
{
	uint2 uv = UVs;
	float wrap = 0.6;
	float scatterWidth = 0.3;
	float4 scatterColor = color;

	// Phong Stuff
	float3 Normal;
	float3 Light;
	float3 Eye;

	Normal = normalize(rotNorm);
	Light = normalize(lightVec);
	Eye = normalize(eyeVec);

	//set ambient
	float4 ambient = color * .6;

		//set diffuse
	float d; 
	if(mode & DIFFUSE_WRAP) d = max((dot(Normal, Light) + wrap) / (1 + wrap), 0);
	else d = max(dot(Normal, Light), 0);
	float4 diffuse = 2 * d * color;

	float s = 0.;

	if (dot(Normal, Light) > 0)
	{
		float3 ref = normalize(2. * Normal * dot(Normal, Light) - Light);
			s = pow(max(dot(Eye, ref), 0), 500);
	}
	float4 specular = 2.5 * s * lightCol;

	if (!(mode & PHONG_RENDER))
	{
		uint2 lightCoords;

		lightCoords.x = fromLightX[uv];
		lightCoords.y = fromLightY[uv];

		float mdepth = uvDepth[uv];

		float shallow = Shallow[lightCoords];

		if ((mode & PIXSYNC_OFF) && (uvDepth[uv] > Shallow[lightCoords])) diffuse -= (mdepth - shallow) * 1.5;
	}

	color = float4(ambient.rgb + diffuse.rgb + specular.rgb, 1.);
		return color;
}