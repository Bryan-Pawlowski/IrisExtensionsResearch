#include<vector>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

struct VERTEX { 

	FLOAT X, Y, Z; 
	D3DXVECTOR3 Normal; 

};

struct VEC3 {
	FLOAT x, y, z;
};

struct VEC2 {
	FLOAT u, v;

};

struct VEC3UINT {
	UINT members[3];
};

struct FACE {
	VEC3UINT points[3];
};

struct model { 
	std::vector<VERTEX> v;		//Vertex (including vertex normals)
	std::vector<VEC2>	vt;		//Texture Coordinates
};


//struct model ReadObject(char* filename);