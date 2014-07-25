#include<vector>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>

struct VERTEX { 

	FLOAT X, Y, Z; 
	D3DXVECTOR3 Normal; 

};

struct VEC3 {
	FLOAT X, Y, Z;
};

struct model { 
		
	std::vector<VERTEX> v;		//Vertex (including vertex normals)
	std::vector<VEC3>	vt;		//Texture Coordinates
	std::vector<int>	i;

};


struct model ReadObject(char* filename);