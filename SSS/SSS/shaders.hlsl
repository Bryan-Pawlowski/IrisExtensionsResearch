#include "./IGFXExtensions/IntelExtensions.hlsl"

#define TEXSIZE 1024.f
#define SCREEN_WIDTH	1280.f
#define SCREEN_HEIGHT	720.f

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

	output.UVs = texCoord;

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
	uv.x = int(TEXSIZE * UVs.x);
	uv.y = int(TEXSIZE * UVs.y);

	float pos = distance(position, camera);
	float mdepth = pos;

	fromLightX[uv] = (float)pixelAddr.x;
	fromLightY[uv] = (float)pixelAddr.y;
	uvDepth[uv] = mdepth;

	IntelExt_Init();

	IntelExt_BeginPixelShaderOrdering();

	float currDepth = Shallow[pixelAddr];
	
	if (pos < currDepth) currDepth = pos;
		
	Shallow[pixelAddr] = currDepth;

	color.g += ((mdepth - currDepth)/5);

	//color.a = .75f;

	return color;
}

double bilinearFilterUVD(float2 uvs){
	double u, v;

	u = uvs.x * TEXSIZE - 0.5;
	v = uvs.y * TEXSIZE - 0.5;


	uint2 xy, x1y, xy1, x1y1;
	xy.x = floor(u);
	xy.y = floor(v);
	x1y = xy;
	xy1 = xy;
	x1y1 = xy;
	x1y.x++;
	xy1.y++;
	x1y1.x++;
	x1y1.y++;


	double u_ratio = u - xy.x;
	double v_ratio = v - xy.y;
	double u_opposite = 1 - u_ratio;
	double v_opposite = 1 - v_ratio;

	double result = (uvDepth[xy] * u_opposite + uvDepth[x1y] * u_ratio) * v_opposite + 
							(uvDepth[xy1] * u_opposite + uvDepth[x1y1] * u_ratio) * v_ratio;

	return result;

}

double bilinearFilterShallow(float2 uvs){
	double u, v;

	u = uvs.x * SCREEN_WIDTH  - 0.5;
	v = uvs.y * SCREEN_HEIGHT - 0.5;


	uint2 xy, x1y, xy1, x1y1;
	xy.x = floor(u);
	xy.y = floor(v);
	x1y = xy;
	xy1 = xy;
	x1y1 = xy;
	x1y.x++;
	xy1.y++;
	x1y1.x++;
	x1y1.y++;


	double u_ratio = u - xy.x;
	double v_ratio = v - xy.y;
	double u_opposite = 1 - u_ratio;
	double v_opposite = 1 - v_ratio;

	double result = (Shallow[xy] * u_opposite + Shallow[x1y] * u_ratio) * v_opposite +
		(Shallow[xy1] * u_opposite + Shallow[x1y1] * u_ratio) * v_ratio;

	return result;

}



float4 PShader2(float4 svposition : SV_POSITION, float4 color : COLOR, float4 position : POSITION, float2 UVs : UV, float4 norm : NORMAL, float4 camera : CAMERA, uint samp : SAMPLE) : SV_TARGET
{
	uint2 uv;
	uv.x = int((TEXSIZE) * UVs.x);
	uv.y = int((TEXSIZE) * UVs.y);



	//todo: do from-scratch sampling on mdepth.

	float tol = .05;

	float2 lightCoords;

	lightCoords.x = fromLightX[uv];
	lightCoords.y = fromLightY[uv];

	float2 SUV;
	SUV.x = lightCoords.x / SCREEN_WIDTH;
	SUV.y = lightCoords.y / SCREEN_HEIGHT;

	double mdepth = bilinearFilterUVD(UVs);
	double shallow = bilinearFilterShallow(SUV);

		if((!((mdepth > (shallow - tol)) && (mdepth < (shallow + tol)))) && (shallow != 100.f)) color.rgb *= 1 - ((mdepth - shallow)*.25);
		
		
		return color;
}

