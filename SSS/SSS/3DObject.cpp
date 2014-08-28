#include "3DObject.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;


struct model ReadObject(char* filename)
{
	struct model out;
	/*
	fstream fs;
	string line;

	D3DXVECTOR3 tempVert;
	D3DXVECTOR2 tex;
	D3DXVECTOR3 tempNorm;
	FACE tempFace;
	vector <D3DXVECTOR3> norms;
	vector <D3DXVECTOR3> verts;
	vector <D3DXVECTOR2> texes;
	UINT ind;
	int vCount = 0, nCount = 0;

	fs.open("car.obj", std::fstream::in );
	
	if (!fs) return out;
	char c, d;

	while (fs >> c){
		switch (c){
		case '#':
			getline(fs, line);
			break;
		case 'v':
			fs.get(d);
			switch (d){
			case ' ':
				fs >> tempVert.x;
				fs >> tempVert.y;
				fs >> tempVert.z;
				verts.push_back(tempVert);
				vCount++;
				break;
			case 'n':
				fs >> tempNorm.x;
				fs >> tempNorm.y;
				fs >> tempNorm.z;
				norms.push_back(tempNorm);
				nCount++;
				break;
			case 't':
				fs >> tex.x;
				fs >> tex.y;
				texes.push_back(tex);
				break;
			default:
				break;
			}
			break;
		case 'f':
			int i = -1, j = 0;

			while (i < 3)
			{
				fs.get(d);
					switch (d){
					case '/':
						break;
					case ' ':
						i++;
						tempFace.points[i].members[j++];
						break;
				}
			}

			break;
		default:
			break;
		}
	}
	fs.close();

	*/
	return out;

}