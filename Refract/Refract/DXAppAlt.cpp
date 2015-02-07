// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <dinput.h>
#include "IGFXExtensions\ID3D10Extensions.h"
#include "IGFXExtensions\IGFXExtensionsHelper.h"
#include "OBJ-Loader.h"
#include <vector>
#include <XNAMath.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800


//Necessary context type thingies.
IDXGISwapChain *swapchain;
ID3D11Device *dev;
ID3D11DeviceContext *devcon;
ID3D11RenderTargetView *backbuffer;
ID3D11DepthStencilView *zbuffer;
ID3D11InputLayout *pLayout;
ID3D11RasterizerState *DisableCull;
ID3D11RasterizerState *CullBack;


//For Skybox
ID3D11Buffer *sphereIndexBuffer;
ID3D11Buffer *sphereVertBuffer;
ID3D11Buffer *pSBCBuffer;
ID3D10Blob	*SKYMAP_VS_BUFFER;
ID3D10Blob	*SKYMAP_PS_BUFFER;
ID3D11VertexShader *SKYMAP_VS;
ID3D11PixelShader *SKYMAP_PS;
ID3D11ShaderResourceView *smrv;
ID3D11SamplerState *CubesTexSamplerState;

D3DXMATRIX sphereWorld;

ID3D11DepthStencilState * DSLessEqual;

//Intel Extension stuff
IGFX::Extensions myExtensions;

//Camera Info
D3DXVECTOR3 camPosition = D3DXVECTOR3(0.0f, 5.0f, -8.0f);
D3DXVECTOR3 camTarget = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
D3DXVECTOR3 camUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

struct cbPerObject
{
	D3DXMATRIX WVP;
	D3DXMATRIX World;
};

cbPerObject cbPerObj;

void InitD3D(HWND hWnd);
void RenderFrame(void);
void CleanD3D(void);
void InitGraphics(void);
void InitPipeline(void);
void InitializeUAVs(void);

void CreateSphere(int latLines, int longLines);

unsigned int NumSphereVertices, NumSphereFaces;


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);



int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	//HWND hMenu;
	WNDCLASSEX wc; // menuWc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";

	if (!RegisterClassEx(&wc))
	{
		int nResult = GetLastError();
		MessageBox(NULL,
			L"DirectX window class creation failed\r\n",
			L"Window Class Failed",
			MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowEx(NULL,
		L"WindowClass",
		L"Pawlowski Pixel Ordering Research",
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hWnd, nCmdShow);

	// set up and initialize Direct3D
	
	InitD3D(hWnd);

	// enter the main loop:

	MSG msg;

	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;

		}

		RenderFrame();
	}

	// clean up DirectX and COM
	CleanD3D();

	//	struct model temp = ReadObject(NULL);
	
	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
					   PostQuitMessage(0);
					   return 0;
	} break;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


void InitD3D(HWND hWnd)
{

	DXGI_SWAP_CHAIN_DESC scd;

	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                   // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // use 32-bit color
	scd.BufferDesc.Width = SCREEN_WIDTH;                   // set the back buffer width
	scd.BufferDesc.Height = SCREEN_HEIGHT;                 // set the back buffer height
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
	scd.OutputWindow = hWnd;                               // the window to be used
	scd.SampleDesc.Count = 4;                              // how many multisamples
	scd.Windowed = TRUE;                                   // windowed/full-screen mode
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    // allow full-screen switching

	// create a device, device context and swap chain using the information in the scd struct
	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&swapchain,
		&dev,
		NULL,
		&devcon);

	if (res != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Unable to initialize D3D!", L"D3D Failure", MB_OK);
		exit(EXIT_FAILURE);
	}

	D3D11_TEXTURE2D_DESC texd;
	ZeroMemory(&texd, sizeof(texd));

	texd.Width = SCREEN_WIDTH;
	texd.Height = SCREEN_HEIGHT;
	texd.ArraySize = 1;
	texd.MipLevels = 1;
	texd.SampleDesc.Count = 4;
	texd.Format = DXGI_FORMAT_D32_FLOAT;
	texd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	/*--Depth Buffer Creation--*/
	ID3D11Texture2D *pDepthBuffer;
	res = dev->CreateTexture2D(&texd, NULL, &pDepthBuffer);
	if (res != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Unable to create depth buffer texture!", L"D3D Failure", MB_OK);
		exit(EXIT_FAILURE);
	}
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));

	dsvd.Format = DXGI_FORMAT_D32_FLOAT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	res = dev->CreateDepthStencilView(pDepthBuffer, &dsvd, &zbuffer);
	pDepthBuffer->Release();
	/*--Depth Buffer Created!--*/
	
	/*------------------------------------------------------------------------------*/

	/*--Back Buffer Creation--*/
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	devcon->OMSetRenderTargets(1, &backbuffer, zbuffer);
	/*--Back Buffer Created and set!--*/

	/*------------------------------------------------------------------------------*/

	/*--Set the Viewport--*/
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCREEN_WIDTH;
	viewport.Height = SCREEN_HEIGHT;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	devcon->RSSetViewports(1, &viewport);
	/*--Our viewport is now set!--*/

	/*------------------------------------------------------------------------------*/

	/*--Check our Intel Extensions!--*/
	res = IGFX::Init(dev);
	if (res == S_OK) myExtensions = IGFX::getAvailableExtensions(dev);
	/*--We now have knowledge of our intel extensions!*/

	/*------------------------------------------------------------------------------*/

	/*--Load Cube Texture--*/
	D3DX11_IMAGE_LOAD_INFO loadSMInfo;
	loadSMInfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	//Load the texture
	ID3D11Texture2D* SMTexture = 0;
	res = D3DX11CreateTextureFromFile(dev, L"skymap.dds",
		&loadSMInfo, 0, (ID3D11Resource**)&SMTexture, 0);

	//Create the texture description
	D3D11_TEXTURE2D_DESC SMTextureDesc;
	SMTexture->GetDesc(&SMTextureDesc);

	//Tell D3DWe have a cube texture, which is an array of 2D textures.
	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = SMTextureDesc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	SMViewDesc.TextureCube.MipLevels = SMTextureDesc.MipLevels;
	SMViewDesc.TextureCube.MostDetailedMip = 0;

	res = dev->CreateShaderResourceView(SMTexture, &SMViewDesc, &smrv);
	
	if (res != S_OK){
		MessageBox(HWND_DESKTOP, L"Could not load the texture cube!", L"Texture error!", MB_OK);
		exit(EXIT_SUCCESS);
	}

	D3D11_SAMPLER_DESC sampdesc;
	ZeroMemory(&sampdesc, sizeof(sampdesc));
	sampdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampdesc.MinLOD = 0;
	sampdesc.MaxLOD = D3D11_FLOAT32_MAX;

	res = dev->CreateSamplerState(&sampdesc, &CubesTexSamplerState);

	if (res != S_OK){
		MessageBox(HWND_DESKTOP, L"Could not create sampler state!", L"Sampler State Error", MB_OK);
	}

	InitPipeline();
	InitGraphics();

}

void InitPipeline(void){
	//shaders go here. Start with the skybox.

	ID3D10Blob *SkyErrors;

	HRESULT Result = D3DX11CompileFromFile(L"skymap.hlsl", 0, 0, "SKYMAP_VS", "vs_5_0", 0, 0, 0, &SKYMAP_VS_BUFFER, &SkyErrors, 0);
	if (Result)
	{
		char *buff = (char *)SkyErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Skybox VS Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	Result = D3DX11CompileFromFile(L"skymap.hlsl", 0, 0, "SKYMAP_PS", "ps_5_0", 0, 0, 0, &SKYMAP_PS_BUFFER, &SkyErrors, 0);
	if (Result)
	{
		char *buff = (char *)SkyErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Skybox PS Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	dev->CreateVertexShader(SKYMAP_VS_BUFFER->GetBufferPointer(), SKYMAP_VS_BUFFER->GetBufferSize(), NULL, &SKYMAP_VS);
	dev->CreatePixelShader(SKYMAP_PS_BUFFER->GetBufferPointer(), SKYMAP_PS_BUFFER->GetBufferSize(), NULL, &SKYMAP_PS);

	/*--Skymap shaders compiled and linked.--*/

	/*------------------------------------------------------------------------------*/

	/*--Define IED (Input Element Description)--*/

	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	Result = dev->CreateInputLayout(ied, 3, SKYMAP_VS_BUFFER->GetBufferPointer(), SKYMAP_VS_BUFFER->GetBufferSize(), &pLayout);
	devcon->IASetInputLayout(pLayout);

	/*--IED Defined and Set.--*/

	/*------------------------------------------------------------------------------*/

	/*--Define and set our constant buffers here.--*/
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(cbPerObject);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Result = dev->CreateBuffer(&bd, NULL, &pSBCBuffer);
	if (Result != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Skybox Constant Buffer Creation unsuccessful!", L"D3D Device Error!", MB_OK);
		exit(EXIT_FAILURE);
	}
	devcon->VSSetConstantBuffers(0, 1, &pSBCBuffer); //We just initially set the constant buffer.
}

void InitGraphics(void){

	/*--Create Skybox--*/
	CreateSphere(20, 20); //Sphere geometry created and stored.

	/*--Rasterizer stuff--*/

	/*--Set a disabled cull state--*/
	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_SOLID;
	wfdesc.CullMode = D3D11_CULL_NONE;
	HRESULT Result = dev->CreateRasterizerState(&wfdesc, &DisableCull);
	if (Result != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Creating Skybox Rasterizer state failed!", L"Graphics init failure!", MB_OK);
		exit(EXIT_FAILURE);
	}
	/*--Disabled Cull state created!--*/

	/*------------------------------------------------------------------------------*/

	/*--Enable Cull Raster State--*/

	wfdesc.CullMode = D3D11_CULL_BACK;
	Result = dev->CreateRasterizerState(&wfdesc, &CullBack);

	/*--Enable Cull Raster State Created!--*/

	/*------------------------------------------------------------------------------*/

	/*--DepthStencil!--*/
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	Result = dev->CreateDepthStencilState(&dsDesc, &DSLessEqual);

	if (Result != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Depth Stencil Failed to create!", L"D3D Error!", MB_OK);
	}

}

void CleanD3D(void)
{
	swapchain->Release();
	dev->Release();
	devcon->Release();

	SKYMAP_PS->Release();
	SKYMAP_VS->Release();
	SKYMAP_PS_BUFFER->Release();
	SKYMAP_VS_BUFFER->Release();

	smrv->Release();

	DisableCull->Release();
	CullBack->Release();
}
void CreateSphere(int LatLines, int LongLines)
{
	NumSphereVertices = ((LatLines - 2) * LongLines) + 2;
	NumSphereFaces = ((LatLines - 3) * (LongLines * 2)) + (LongLines * 2);

	float sphereYaw = 0.0f;
	float spherePitch = 0.0f;

	std::vector<VERTEX> vertices(NumSphereVertices);

	XMMATRIX Rotationx, Rotationy;
	XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	vertices[0].vert.x = 0.0f;
	vertices[0].vert.y = 0.0f;
	vertices[0].vert.z = 1.0f;
	vertices[0].vert.w = .25f;

	vertices[0].norm.x = 0.0f;
	vertices[0].norm.y = 0.0f;
	vertices[0].norm.z = -1.0f;
	vertices[0].texCoords.x = 0.0f;
	vertices[0].texCoords.y = 0.0f;

	for (DWORD i = 0; i < LatLines - 2; ++i)
	{
		spherePitch = (i + 1) * (3.14 / (LatLines - 1));
		Rotationx = XMMatrixRotationX(spherePitch);
		for (DWORD j = 0; j < LongLines; ++j)
		{
			sphereYaw = j * (6.28 / (LongLines));
			Rotationy = XMMatrixRotationZ(sphereYaw);
			currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy));
			currVertPos = XMVector3Normalize(currVertPos);
			vertices[i*LongLines + j + 1].vert.x = XMVectorGetX(currVertPos);
			vertices[i*LongLines + j + 1].vert.y = XMVectorGetY(currVertPos);
			vertices[i*LongLines + j + 1].vert.z = XMVectorGetZ(currVertPos);
			vertices[i*LongLines + j + 1].vert.w = .25f;

			vertices[i*LongLines + j + 1].norm.x = vertices[i*LongLines + j + 1].vert.x;
			vertices[i*LongLines + j + 1].norm.y = vertices[i*LongLines + j + 1].vert.y;
			vertices[i*LongLines + j + 1].norm.z = vertices[i*LongLines + j + 1].vert.z;
			vertices[i*LongLines + j + 1].texCoords.x = (float)i / (float)LatLines;
			vertices[i*LongLines + j + 1].texCoords.y = (float)j / (float)LongLines;
		}
	}

	vertices[NumSphereVertices - 1].vert.x = 0.0f;
	vertices[NumSphereVertices - 1].vert.y = 0.0f;
	vertices[NumSphereVertices - 1].vert.z = -1.0f;
	vertices[NumSphereVertices - 1].vert.w = .25f;

	vertices[NumSphereVertices - 1].norm.x = 0.0f;
	vertices[NumSphereVertices - 1].norm.y = 0.0f;
	vertices[NumSphereVertices - 1].norm.z = 1.0f;
	vertices[NumSphereVertices - 1].texCoords.x = 1.0f;
	vertices[NumSphereVertices - 1].texCoords.y = 1.0f;

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VERTEX)* NumSphereVertices;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = dev->CreateBuffer(&vertexBufferDesc, NULL, &sphereVertBuffer);

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = vertices._Myfirst;
	//HRESULT hr = dev->CreateBuffer(&vertexBufferDesc, NULL, &sphereVertBuffer);

	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(sphereVertBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
	memcpy(ms.pData, vertices._Myfirst, sizeof(VERTEX)* vertices.size());                 // copy the data
	devcon->Unmap(sphereVertBuffer, NULL);

	if (hr != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Sphere creation failed", L"Sphere Error!", MB_OK);
		exit(EXIT_FAILURE);
	}


	std::vector<DWORD> indices(NumSphereFaces * 3);

	int k = 0;
	for (DWORD l = 0; l < LongLines - 1; ++l)
	{
		indices[k] = 0;
		indices[k + 1] = l + 1;
		indices[k + 2] = l + 2;
	}

	indices[k] = 0;
	indices[k + 1] = LongLines;
	indices[k + 2] = 1;
	k += 3;

	for (DWORD i = 0; i < LatLines - 3; ++i)
	{
		for (DWORD j = 0; j < LongLines - 1; ++j)
		{
			indices[k] = i*LongLines + j + 1;
			indices[k + 1] = i*LongLines + j + 2;
			indices[k + 2] = (i + 1)*LongLines + j + 1;

			indices[k + 3] = (i + 1)*LongLines + j + 1;
			indices[k + 4] = i*LongLines + j + 2;
			indices[k + 5] = (i + 1)*LongLines + j + 2;

			k += 6; // next quad
		}

		indices[k] = (i*LongLines) + LongLines;
		indices[k + 1] = (i*LongLines) + 1;
		indices[k + 2] = ((i + 1)*LongLines) + LongLines;

		indices[k + 3] = ((i + 1)*LongLines) + LongLines;
		indices[k + 4] = (i*LongLines) + 1;
		indices[k + 5] = ((i + 1)*LongLines) + 1;

		k += 6;
	}

	for (DWORD l = 0; l < LongLines - 1; ++l)
	{
		indices[k] = NumSphereVertices - 1;
		indices[k + 1] = (NumSphereVertices - 1) - (l + 1);
		indices[k + 2] = (NumSphereVertices - 1) - (l + 2);
		k += 3;
	}

	indices[k] = NumSphereVertices - 1;
	indices[k + 1] = (NumSphereVertices - 1) - LongLines;
	indices[k + 2] = NumSphereVertices - 2;

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	indexBufferDesc.ByteWidth = sizeof(DWORD)* NumSphereFaces * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = indices._Myfirst;
	HRESULT hres = dev->CreateBuffer(&indexBufferDesc, NULL, &sphereIndexBuffer);


	D3D11_MAPPED_SUBRESOURCE ims;
	devcon->Map(sphereIndexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ims);    // map the buffer
	memcpy(ims.pData, indices._Myfirst, sizeof(DWORD)* indices.size());                 // copy the data
	devcon->Unmap(sphereIndexBuffer, NULL);

	if (hr != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Problem Creating Sphere!", L"Sphere Creation Failed", MB_OK);
		exit(EXIT_FAILURE);
	}

}

void RenderFrame()
{
	D3DXMATRIX ModelViewProjection, matProjection, lookAt;
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;

	devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

	D3DXMatrixIdentity(&sphereWorld);

	D3DXMATRIX scale, translation;
	D3DXMatrixScaling(&scale, 5.0f, 5.0f, 5.0f);
	D3DXMatrixIdentity(&translation);

	sphereWorld = scale * translation;

	D3DXMatrixLookAtLH(&lookAt, &camPosition, &camTarget, &camUp);
	D3DXMatrixPerspectiveFovLH(&matProjection,
		0.4f*3.14f,                    // field of view
		(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_WIDTH, // aspect ratio
		1.0f,                                       // near view-plane
		1000.0f);                                    // far view-plane
	ModelViewProjection = sphereWorld * lookAt * matProjection;

	devcon->IASetIndexBuffer(sphereIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	devcon->IASetVertexBuffers(0, 1, &sphereVertBuffer, &stride, &offset);

	cbPerObj.WVP = ModelViewProjection;
	cbPerObj.World = sphereWorld;

	devcon->UpdateSubresource(pSBCBuffer, 0, NULL, &cbPerObj, 0, 0);
	devcon->PSSetShaderResources(0, 1, &smrv);
	devcon->PSSetSamplers(0, 1, &CubesTexSamplerState);

	devcon->VSSetShader(SKYMAP_VS, 0, 0);
	devcon->PSSetShader(SKYMAP_PS, 0, 0);

	devcon->OMSetDepthStencilState(DSLessEqual, 0);
	devcon->RSSetState(DisableCull);

	devcon->DrawIndexed(NumSphereFaces * 3, 0, 0);

	swapchain->Present(0, 0);
}