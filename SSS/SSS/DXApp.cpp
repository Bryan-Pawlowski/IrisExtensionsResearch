// include the basic windows header files and the Direct3D header files
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <array>
#include "IGFXExtensions\ID3D10Extensions.h"
#include "IGFXExtensions\IGFXExtensionsHelper.h"

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

// define the screen resolution
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600


// global declarations
IDXGISwapChain *swapchain;				// the pointer to the swap chain interface
ID3D11Device *dev;						// the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;			// the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;		// the pointer to our back buffer
ID3D11DepthStencilView *zbuffer;		// the pointer to our depth buffer
ID3D11InputLayout *pLayout;				// the pointer to the input layout
ID3D11VertexShader *pVS;				// the pointer to the vertex shader
ID3D11PixelShader *pPS;					// the pointer to the pixel shader
ID3D11Buffer *pVBuffer;					// the pointer to the vertex buffer
ID3D11Buffer *pIBuffer;					// the pointer to the index buffer
ID3D11Buffer *pCBuffer;					// the pointer to the constant buffer
IGFX::Extensions myExtensions;
ID3D11RasterizerState* RenderState;		// We are setting the Rasterizer state, at this point, to disable culling.
ID3D11BlendState* g_pBlendState;
ID3D11UnorderedAccessView *pUAV;		// Our application-side UAV entity.
ID3D11Texture2D *pUAVTex;				// Our application-side definition of data stored in UAV.

// a struct to define a single vertex
struct VERTEX { FLOAT X, Y, Z; D3DXVECTOR3 Normal; };

// a struct to define the constant buffer
struct CBUFFER
{
	D3DXMATRIX Final;
	D3DXMATRIX Rotation;
	D3DXMATRIX modelView;
	D3DXVECTOR4 LightVector;
	D3DXCOLOR LightColor;
	D3DXCOLOR AmbientColor;
};

// function prototypes
void InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void RenderFrame(void);     // renders a single frame
void CleanD3D(void);        // closes Direct3D and releases memory
void InitGraphics(void);    // creates the shape to render
void InitPipeline(void);    // loads and prepares the shaders

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx(&wc);

	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowEx(NULL,
		L"WindowClass",
		L"Pawlowski Subsurface Scattering Model",
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

	return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
					   PostQuitMessage(0);
					   return 0;
	} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
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
	D3D11CreateDeviceAndSwapChain(NULL,
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


	// create the depth buffer texture
	D3D11_TEXTURE2D_DESC texd;
	ZeroMemory(&texd, sizeof(texd));

	texd.Width = SCREEN_WIDTH;
	texd.Height = SCREEN_HEIGHT;
	texd.ArraySize = 1;
	texd.MipLevels = 1;
	texd.SampleDesc.Count = 4;
	texd.Format = DXGI_FORMAT_D32_FLOAT;
	texd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D *pDepthBuffer;
	dev->CreateTexture2D(&texd, NULL, &pDepthBuffer);

	
	// create the depth buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));

	dsvd.Format = DXGI_FORMAT_D32_FLOAT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	dsvd.Texture2D.MipSlice = 5;

	HRESULT res = dev->CreateDepthStencilView(pDepthBuffer, &dsvd, &zbuffer);
	pDepthBuffer->Release();

	// get the address of the back buffer
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// use the back buffer address to create the render target
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);

	// set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, zbuffer);


	// Set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;    // set the left to 0
	viewport.TopLeftY = 0;    // set the top to 0
	viewport.Width = SCREEN_WIDTH;    // set the width to the window's width
	viewport.Height = SCREEN_HEIGHT;    // set the height to the window's height
	viewport.MinDepth = 0;    // the closest an object can be on the depth buffer is 0.0
	viewport.MaxDepth = 1;    // the farthest an object can be on the depth buffer is 1.0

	devcon->RSSetViewports(1, &viewport);

	HRESULT	IntelResult = IGFX::Init(dev);										//initialize our Iris Extensions
	if (IntelResult == S_OK) myExtensions = IGFX::getAvailableExtensions(dev);	//check what we have available and store it in a global (for checks)


	// Since this texture will technically be "multisampled," we don't need to pass any initial data. 

	//create UAV for the pixel shaders to use

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc;

	UAVdesc.Format = DXGI_FORMAT_R32_UINT;
	UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	
	HRESULT UAVRes = dev->CreateUnorderedAccessView( pBackBuffer, &UAVdesc, &pUAV);
	if (UAVRes != S_OK){
		MessageBox(HWND_DESKTOP, L"Our UAV view was not successful..." L"Vertex Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}


	pBackBuffer->Release();



	InitPipeline();
	InitGraphics();
}


// this is the function used to render a single frame
void RenderFrame(void)
{
	CBUFFER cBuffer;

	cBuffer.LightVector = D3DXVECTOR4(0.5f, 0.75f, 0.25f, 0.0f);
	cBuffer.LightColor = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	cBuffer.AmbientColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);

	D3DXMATRIX matRotate, matView, matProjection;
	D3DXMATRIX matFinal;

	static float Time = 0.0f; Time += 0.0003f;

	// create a world matrices
	D3DXMatrixRotationY(&matRotate, Time);

	// create a view matrix
	D3DXMatrixLookAtLH(&matView,
		&D3DXVECTOR3(0.0f, 3.0f, 5.0f),   // the camera position
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

	// create a projection matrix
	D3DXMatrixPerspectiveFovLH(&matProjection,
		(FLOAT)D3DXToRadian(45),                    // field of view
		(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
		1.0f,                                       // near view-plane
		100.0f);                                    // far view-plane

	// load the matrices into the constant buffer
	cBuffer.Final = matRotate * matView * matProjection;
	cBuffer.Rotation = matRotate;
	cBuffer.modelView = matView;

	// clear the back buffer to a deep blue
	devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

	// clear the depth buffer
	devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// select which vertex buffer to display
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
	devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);

	// select which primtive type we are using
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// draw the Hypercraft
	devcon->UpdateSubresource(pCBuffer, 0, 0, &cBuffer, 0, 0);
	devcon->DrawIndexed(36, 0, 0);

	// switch the back buffer and the front buffer
	swapchain->Present(0, 0);
}


// this is the function that cleans up Direct3D and COM
void CleanD3D(void)
{
	swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

	// close and release all existing COM objects
	zbuffer->Release();
	pLayout->Release();
	pVS->Release();
	pPS->Release();
	pVBuffer->Release();
	pIBuffer->Release();
	pCBuffer->Release();
	swapchain->Release();
	backbuffer->Release();
	RenderState->Release();
	dev->Release();
	devcon->Release();
}


// this is the function that creates the shape to render
void InitGraphics()
{
	// create vertices to represent the corners of the cube
	VERTEX OurVertices[] =
	{
		{ -1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f) },    // side 1
		{ 1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f) },
		{ -1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f) },
		{ 1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 0.0f, 1.0f) },

		{ -1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f) },    // side 2
		{ -1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f) },
		{ 1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f) },
		{ 1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 0.0f, -1.0f) },

		{ -1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f) },    // side 3
		{ -1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f) },
		{ 1.0f, 1.0f, -1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f) },
		{ 1.0f, 1.0f, 1.0f, D3DXVECTOR3(0.0f, 1.0f, 0.0f) },

		{ -1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f) },    // side 4
		{ 1.0f, -1.0f, -1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f) },
		{ -1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f) },
		{ 1.0f, -1.0f, 1.0f, D3DXVECTOR3(0.0f, -1.0f, 0.0f) },

		{ 1.0f, -1.0f, -1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f) },    // side 5
		{ 1.0f, 1.0f, -1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f) },
		{ 1.0f, -1.0f, 1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f) },
		{ 1.0f, 1.0f, 1.0f, D3DXVECTOR3(1.0f, 0.0f, 0.0f) },

		{ -1.0f, -1.0f, -1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f) },    // side 6
		{ -1.0f, -1.0f, 1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f) },
		{ -1.0f, 1.0f, -1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f) },
		{ -1.0f, 1.0f, 1.0f, D3DXVECTOR3(-1.0f, 0.0f, 0.0f) },
	};

	// create the vertex buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX)* 24;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	dev->CreateBuffer(&bd, NULL, &pVBuffer);

	// copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
	memcpy(ms.pData, OurVertices, sizeof(OurVertices));                 // copy the data
	devcon->Unmap(pVBuffer, NULL);


	// create the index buffer out of DWORDs
	DWORD OurIndices[] =
	{
		0, 1, 2,    // side 1
		2, 1, 3,
		4, 5, 6,    // side 2
		6, 5, 7,
		8, 9, 10,    // side 3
		10, 9, 11,
		12, 13, 14,    // side 4
		14, 13, 15,
		16, 17, 18,    // side 5
		18, 17, 19,
		20, 21, 22,    // side 6
		22, 21, 23,
	};

	// create the index buffer
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DWORD)* 36;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;

	dev->CreateBuffer(&bd, NULL, &pIBuffer);

	devcon->Map(pIBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
	memcpy(ms.pData, OurIndices, sizeof(OurIndices));                   // copy the data
	devcon->Unmap(pIBuffer, NULL);

	D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_SOLID;
	wfdesc.CullMode = D3D11_CULL_NONE;
	dev->CreateRasterizerState(&wfdesc, &RenderState);

	devcon->RSSetState(RenderState);

	//set the depth stencil state.

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test params
	dsDesc.DepthEnable = false;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// If Pixel is front-facing...
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	//if pixel is back-facing...

	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;


	// Now after describing the state, we want to create and bind it to the device context.

	ID3D11DepthStencilState * pDSState;
	HRESULT res = dev->CreateDepthStencilState(&dsDesc, &pDSState);


	//Set up blending state

	D3D11_BLEND_DESC BlendState;

	ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
	BlendState.RenderTarget[0].BlendEnable = TRUE;
	BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendState.RenderTarget[0].BlendOpAlpha= D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	dev->CreateBlendState(&BlendState, &g_pBlendState);

	devcon->OMSetDepthStencilState(pDSState, 1);

	float blendfactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	UINT sampleMask = 0xffffffff;

	devcon->OMSetBlendState( g_pBlendState, blendfactor, sampleMask);

}


// this function loads and prepares the shaders
void InitPipeline()
{
	// compile the shaders
	ID3D10Blob *VS, *PS, *VErrors, *PErrors;
	HRESULT Result;


	Result = D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "VShader", "vs_4_0", 0, 0, 0, &VS, &VErrors, 0);
	if (Result)
	{
		char *buff = (char *)VErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Vertex Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}
	Result = D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "PShader", "ps_5_0", 0, 0, 0, &PS, &PErrors, 0);
	if (Result)
	{
		char *buff = (char *)PErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Pixel Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}
	// create the shader objects
	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

	// set the shader objects
	devcon->VSSetShader(pVS, 0, 0);
	devcon->PSSetShader(pPS, 0, 0);

	// create the input element object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
	devcon->IASetInputLayout(pLayout);

	// create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CBUFFER);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	dev->CreateBuffer(&bd, NULL, &pCBuffer);

	devcon->VSSetConstantBuffers(0, 1, &pCBuffer);
}

