
//for now, we will assume that our object is directly in front of the light.

#define nodes 2

struct pixelInfo
{
	float zPos, lightIntensity;
};


RWTexture2D<float> lightUAV				: register ( u2 );
