#include "./IGFXExtensions/IntelExtensions.hlsl"

cbuffer cbPerObject
{
	float4x4 WVP;
	float4x4 World;
	float4x4 Rotation;
	float4 LightVector;
	float4 LightColor;
	float4 AmbientColor;
};


struct VOUT
{
	float4 svPos : SV_POSITION;
	float4 Pos : POSITION;
	float4 Normal : NORMAL;
	float4 Color : COLOR;
	float3 vRef : NORMAL1;
	float2 texCoord : TEXCOORD;
};


TextureCube SkyMap;
SamplerState ObjSamplerState;


VOUT VShader(float4 pos : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{

	VOUT output;

	output.svPos = mul(WVP, pos);
	output.Pos = mul(WVP, pos);
	float4 norm = mul(Rotation, normal);
	float diffusebrightness = saturate(dot(norm, LightVector));

	float3 ECPosition = mul(World, pos).xyz;
	float3 eyeDir = float3(0.0f, 0.0f, 0.0f) - ECPosition;
	
	output.Color = AmbientColor;

	output.Color += LightColor * diffusebrightness;
	output.Normal = norm;
	output.texCoord = texCoord;

	float3 reflectVector = refract(norm, eyeDir, -.90);

		output.vRef = reflectVector;


	return output;
}

RWTexture2D<unsigned int>ClearMask	:	register(u1);
RWTexture3D<float3>VoxMap			:	register(u2);
RWTexture3D<unsigned int>VoxMask	:	register(u3);
RWTexture2D<float3>BackNorms		:	register(u4);

float4 PShader( VOUT input ) : SV_TARGET
{
	

	float3 bounceVec = input.vRef.xyz;
	float4 newColor = SkyMap.Sample(ObjSamplerState, normalize(bounceVec));
	return newColor;

}

float4 PVoxelize( VOUT input ) : SV_TARGET
{
	//convert our Position into voxel indices.
	float4 pos = input.Pos;
	uint3 voxPos;
	float3 bounceVec = input.vRef.xyz;
	float3 normal = input.Normal.xyz;
	uint3 dimensions;



	voxPos = input.svPos.xyz;

	VoxMask.GetDimensions(dimensions.x, dimensions.y, dimensions.z);
	
	IntelExt_Init();
	IntelExt_BeginPixelShaderOrdering();

	//set clearMask, if it hasn't been, already.

	//we will loop through the voxel mask to determine where the inside of the model is.
	/*	/--number meanings--/
		0: Outside of Model
		1: Model Edge
		2: Inside Model
	*/

	VoxMap[voxPos] = normal; //store this into the vox map.

	uint i;
	uint3 tempi = voxPos;
	uint tempArr[512];
	VoxMask[voxPos] = 1;//we are at the edge of the model

	[loop]
	for (i = voxPos.z; i < 512; i++){
		tempi.z = i;
		uint temp = VoxMask[tempi];
		tempArr[i] = temp;
	}

	[loop]
	for (i = voxPos.z + 1; i < 512; i++)
	{
		tempi.z = i;

		if (tempArr[i] == 0)
		{
			VoxMask[tempi] = 2;
			continue;
		}
		break;
		
	}
	
	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

