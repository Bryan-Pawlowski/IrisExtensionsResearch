
//for now, we will assume that our object is directly in front of the light.

#define nodes 2

struct pixelInfo
{
	float zPos, lightIntensity;
};

RWTexture2D<float> depthUAV				: register ( u1 );
RWTexture2D<float> lightUAV				: register ( u2 );
RWTexture2D<uint>  pixelTouched			: register ( ps_5_0, u3 );