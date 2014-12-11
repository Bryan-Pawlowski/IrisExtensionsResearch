// include the basic windows header files and the Direct3D header files


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


// define the screen resolution
#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	600


#define MODE_FROMLIGHT					0	//one render and show the scale of the depth from the lightsource.
#define MODE_PERSP_LIGHTDEPTH			1	//two renders and show the light depth from second render.
#define MODE_PERSP_SHOW_X				2	//two renders and show the x coordinates with respect to light.
#define MODE_PERSP_SHOW_Y				4	//two renders and show the y coordinates with respect to light.
#define MODE_PERSP_SHOW_X_AND_Y			6	//two renders and show the x and y coordinates with respect to light.
#define MODE_PERSP_SHOW_ALPHA_SCALE		8	//two renders and show the scale of alpha as depth increases.
#define MODE_PERSP_SHOW_FLAT_SCALE		16	//two renders and show just the flat colorchange without taking phone illumination into account.
#define MODE_PERSP_SHOW_FINAL			32	//two renders and show the composite working image.
#define MODE_SAMPLE						64	//custom sample on second render
#define MODE_NO_SAMPLE					128	//no custom sample on second render


// global declarations
IDXGISwapChain *swapchain;				// the pointer to the swap chain interface
ID3D11Device *dev;						// the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;			// the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;		// the pointer to our back buffer
ID3D11DepthStencilView *zbuffer;		// the pointer to our depth buffer
ID3D11InputLayout *pLayout;				// the pointer to the input layout
ID3D11VertexShader *pBVS;				// the pointer to the bad vertex shader
ID3D11PixelShader *pBPS;				// the pointer to the bad pixel shader in the first pass.
ID3D11PixelShader *pBPS2;				// the pointer to the bad pixel shader in the second pass.
ID3D11VertexShader *pVS;				// the pointer to the vertex shader
ID3D11VertexShader *pVS2;				// the pointer to a second vertex shader
ID3D11PixelShader *pPS;					// the pointer to the pixel shader
ID3D11PixelShader *pPS2;				// the pointer to the second pixel shader
ID3D11PixelShader *pPSO;
ID3D11Buffer *pVBuffer;					// the pointer to the vertex buffer
ID3D11Buffer *pIBuffer;					// the pointer to the index buffer
ID3D11Buffer *pCBuffer;					// the pointer to the constant buffer
IGFX::Extensions myExtensions;
ID3D11RasterizerState* DisableCull;		// We are setting the Rasterizer state, at this point, to disable culling.
ID3D11RasterizerState* EnableCull;		// We enable culling for our second pass.
ID3D11BlendState* g_pBlendState;
ID3D11UnorderedAccessView *pUAV[4];		// Our application-side UAV entity.
ID3D11Texture2D *pUAVTex;				// Our application-side definition of data stored in UAV.
ID3D11Texture2D *pUAVDTex;
ID3D11Texture2D *pUAVDTex2;
ID3D11Texture2D *pUAVDTex3;
ID3D11Texture2D *depthTex;
ID3D11RenderTargetView *depthTexBuff;
ID3D11ShaderResourceView *SRVs[4];
ID3D11DepthStencilState * pDSState;
ID3D11DepthStencilState * pDefaultState;


ID3D11RenderTargetView *RTVs[2];

// a struct to define a single vertex

Model *myModel;  //test use of my simple model class.
ID3D11Buffer *pModelBuffer; //this model buffer can be stored within an object class, once I have that functionality.



D3DXVECTOR4 Light = D3DXVECTOR4(1.5f, 2.2f, 1.75f, 1.0);
unsigned int displayMode = MODE_FROMLIGHT | MODE_SAMPLE; //how to render scene, and whether or not we will use sampling.
bool rotate = true;
bool bRender = false;


unsigned int cowVerts;


// a struct to define the constant buffer
struct CBUFFER
{
	D3DXMATRIX Final;
	D3DXMATRIX Rotation;
	D3DXMATRIX modelView;
	D3DXVECTOR4 LightVector;
	D3DXCOLOR LightColor;
	D3DXCOLOR AmbientColor;
	D3DXVECTOR4 Camera;
	unsigned int mode;
	unsigned int pad[3];
};

typedef struct tagKeyboard
{
	byte state[256];
}keyboard_t;


keyboard_t key;

extern LPDIRECTINPUT			lpdi;		//direct input interface
extern LPDIRECTINPUTDEVICE		lpKeyboard;

// function prototypes
void InitD3D(HWND hWnd);			// sets up and initializes Direct3D
void RenderFrame(void);				// renders a single frame, basic with just a quick depth-based shadow on the object.
void BadRenderFrame(void);			// renders a single frame, but with the bad approach.
void CleanD3D(void);				// closes Direct3D and releases memory
void InitGraphics(void);			// creates the shape to render
void InitPipeline(void);			// loads and prepares the shaders
void PhongRender(void);				// set up render for phong lighting
void RefractRender(void);			// set up render for refraction. This will support both original and new refraction.
void InitializeUAVs(void);			// UAVs get their own initializer now. There was too much to keep track of in the InitD3D() function.
void SingleBounceRTRender(void);	// render call for the single bounce ray trace.

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	HWND hMenu;
	WNDCLASSEX wc, menuWc;

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




	ZeroMemory(&menuWc, sizeof(WNDCLASSEX));

	menuWc.cbClsExtra = NULL;
	menuWc.cbSize = sizeof(WNDCLASSEX);
	menuWc.cbWndExtra = NULL;
	menuWc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	menuWc.hCursor = LoadCursor(NULL, IDC_ARROW);
	menuWc.hIcon = NULL;
	menuWc.hIconSm = NULL;
	menuWc.hInstance = hInstance;
	menuWc.lpfnWndProc = (WNDPROC)MenuProc;
	menuWc.lpszClassName = L"MenuClass";
	menuWc.lpszMenuName = NULL;
	menuWc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&menuWc))
	{
		int nResult = GetLastError();
		MessageBox(NULL,
			L"Window class creation failed\r\n",
			L"Window Class Failed",
			MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	hMenu = CreateWindowEx(NULL,
		L"MenuClass",
		L"Mode Select",
		WS_OVERLAPPEDWINDOW,
		200,
		200,
		640,
		480,
		NULL,
		NULL,
		hInstance,
		NULL);


	if (!hMenu)
	{
		int nResult = GetLastError();

		MessageBox(NULL,
			L"Window creation failed\r\n",
			L"Window Creation Failed",
			MB_ICONERROR);
		exit(EXIT_SUCCESS);
	}

	ShowWindow(hMenu, nCmdShow);

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

		if(!bRender) RenderFrame();
		else BadRenderFrame();
	}

	// clean up DirectX and COM
	CleanD3D();

//	struct model temp = ReadObject(NULL);

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
	case WM_CHAR:
	{
		switch (wParam)
		{
		case '1':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_FROMLIGHT;
			break;
		case '2':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_LIGHTDEPTH;
			break;
		case '3':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_SHOW_X;
			break;
		case '4':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_SHOW_Y;
			break;
		case '5':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_SHOW_X_AND_Y;
			break;
		case '6':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_SHOW_ALPHA_SCALE;
			break;
		case '7':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_SHOW_FLAT_SCALE;
			break;
		case '8':
			displayMode = displayMode & 0xc0;
			displayMode = displayMode | MODE_PERSP_SHOW_FINAL;
			break;
		case 's':
			displayMode = displayMode & 0x3f; //0b00111111
			displayMode = displayMode | MODE_SAMPLE;
			break;
		case 'n':
			displayMode = displayMode & 0x3f; //retain the first eight bits, but clear the last two.
			displayMode = displayMode | MODE_NO_SAMPLE;
			break;
		case ' ':
			rotate = !rotate;
			break;
		case 'b':
		case 'B':
			bRender = !bRender;
			break;
		default:
			break;
		}
	}break;
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case 37:			//left arrowkey pushed
			Light.x -= .03;
			break;
		case 38:			//up arrow key pushed
			Light.y += .03;
			break;
		case 39:			//right arrow key pushed
			Light.x += .03;
			break;
		case 40:			//down arrow key pushed
			Light.y -= .03;
			break;

		default:
			break;
		}
	} break;
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
// this function initializes and prepares Direct3D for use

LRESULT CALLBACK MenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
	//initialize our keyboard.
	
	
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

	HRESULT res = dev->CreateDepthStencilView(pDepthBuffer, &dsvd, &zbuffer);
	pDepthBuffer->Release();

	// get the address of the back buffer
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// use the back buffer address to create the render target
	dev->CreateRenderTargetView(pBackBuffer, NULL, &RTVs[0]);

	// set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &RTVs[0], zbuffer);


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

	InitializeUAVs();

	HRESULT	IntelResult = IGFX::Init(dev);										//initialize our Iris Extensions
	if (IntelResult == S_OK) myExtensions = IGFX::getAvailableExtensions(dev);	//check what we have available and store it in a global (for checks)

	pBackBuffer->Release();



	InitPipeline();
	InitGraphics();
}
// this is the function used to render a single frame
void RenderFrame(void)
{
	CBUFFER cBuffer;

	cBuffer.LightVector = Light;
	cBuffer.LightColor = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	cBuffer.AmbientColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
	cBuffer.Camera = Light;
	cBuffer.mode = displayMode;

	D3DXMATRIX matRotate, matView, matProjection;
	D3DXMATRIX matFinal;

	const float fClear[4] = { -1.f, -1.f, -1.f, -1.f };
	const float dfClear[4] = { 100.f, 100.f, 100.f, 100.f };
	devcon->ClearUnorderedAccessViewFloat(pUAV[0], dfClear);
	devcon->ClearUnorderedAccessViewFloat(pUAV[1], dfClear);

	devcon->ClearUnorderedAccessViewFloat(pUAV[2], fClear);
	devcon->ClearUnorderedAccessViewFloat(pUAV[3], fClear);
	static float Time = 0.0f; 
	
	if(rotate) Time += 0.004;
	
	//Begin First Pass

	devcon->VSSetShader(pVS, 0, 0);
	devcon->PSSetShader(pPS, 0, 0);

	ID3D11RenderTargetView *pNullRTView[] = { NULL };

	//devcon->OMSetRenderTargets(1, pNullRTView, zbuffer);

	devcon->OMSetRenderTargetsAndUnorderedAccessViews(1, &RTVs[0], NULL, 1, 4, pUAV, 0);

	devcon->RSSetState(DisableCull);
	devcon->OMSetDepthStencilState(pDSState, 1);

	// create a world matrices
	D3DXMatrixRotationY(&matRotate, Time);

	// create a view matrix
	D3DXMatrixLookAtLH(&matView,
		&D3DXVECTOR3(Light.x, Light.y, Light.z),   // the camera position // - pass 1: change name of "Camera" to "Light"
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

	// create a projection matrix
	D3DXMatrixOrthoLH(&matProjection, 10, 10, 0, 1);
	/*D3DXMatrixPerspectiveFovLH(&matProjection,
		(FLOAT)D3DXToRadian(90),                    // field of view
		(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_WIDTH, // aspect ratio
		1.0f,                                       // near view-plane
		100.0f);                                    // far view-plane
		*/
	// load the matrices into the constant buffer
	cBuffer.Final = matRotate * matView * matProjection;
	cBuffer.Rotation = matRotate;
	cBuffer.modelView = matView;

	// clear the back buffer to a deep blue
	devcon->ClearRenderTargetView(RTVs[0], D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

	// clear the depth buffer
	devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// select which vertex buffer to display
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	
	//Default object use 
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
	devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);

	//devcon->IASetVertexBuffers(0, 1, &pModelBuffer, &stride, &offset);

	// select which primtive type we are using
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// draw the Hypercraft
	devcon->UpdateSubresource(pCBuffer, 0, 0, &cBuffer, 0, 0);
	devcon->DrawIndexed(36, 0, 0); //this is for the default cube object
	//devcon->Draw(cowVerts, 0);


	//end of first pass.
	//second pass for shallow info

	devcon->VSSetShader(pVS2, 0, 0);
	devcon->PSSetShader(pPSO, 0, 0);

	devcon->ClearRenderTargetView(RTVs[0], D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

	devcon->DrawIndexed(36, 0, 0);
	//devcon->Draw(cowVerts, 0);


	if ((displayMode & MODE_PERSP_SHOW_FLAT_SCALE)){
		//begin second pass.

		D3DXMatrixLookAtLH(&matView,
			&D3DXVECTOR3(-4.0f, 5.0f, -5.0f),   // the camera position
			&D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
			&D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

		// create a projection matrix
		//D3DXMatrixOrthoLH(&matProjection, 4, 4, 0, 1);
		// create a projection matrix
		D3DXMatrixPerspectiveFovLH(&matProjection,
			(FLOAT)D3DXToRadian(45),                    // field of view
			(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
			1.0f,                                       // near view-plane
			100.0f);

		// load the matrices into the constant buffer
		cBuffer.Final = matRotate * matView * matProjection;
		cBuffer.Rotation = matRotate;
		cBuffer.modelView = matView;

		devcon->OMSetRenderTargetsAndUnorderedAccessViews(1, &RTVs[0], zbuffer, 1, 4, pUAV, 0);

		devcon->RSSetState(EnableCull);
		devcon->OMSetDepthStencilState(pDefaultState, 1);
		devcon->ClearRenderTargetView(RTVs[0], D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));
		devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

		devcon->PSSetShader(pPS2, 0, 0);

		devcon->UpdateSubresource(pCBuffer, 0, 0, &cBuffer, 0, 0);
		devcon->DrawIndexed(36, 0, 0);
		//devcon->Draw(cowVerts, 0);
	}

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
	pPS2->Release();
	pVS2->Release();
	pPSO->Release();
	pModelBuffer->Release();
	pVBuffer->Release();
	pIBuffer->Release();
	pCBuffer->Release();
	swapchain->Release();
	RTVs[0]->Release();
	DisableCull->Release();
	EnableCull->Release();
	dev->Release();
	devcon->Release();
}
// this is the function that creates the shape to render
void InitGraphics(void)
{
	// create vertices to represent the corners of the cube
	VERTEX OurVertices[] =
	{
		{ D3DXVECTOR4(-1.0f, -1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR2(0.0f, 0.5f) },    // side 1 "front"
		{ D3DXVECTOR4(1.0f, -1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR2( 0.33f, 0.5f) },
		{ D3DXVECTOR4(-1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR2( 0.0f, 0.0f) },
		{ D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXVECTOR2( 0.33f, 0.0f) },

		{ D3DXVECTOR4(-1.0f, -1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2( 0.33f, 0.5f ) },    // side 2 "back"
		{ D3DXVECTOR4(-1.0f, 1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2( 0.33f, 0.0f ) },
		{ D3DXVECTOR4(1.0f, -1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2( 0.66f, 0.5f ) },
		{ D3DXVECTOR4(1.0f, 1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2( 0.66f, 0.0f ) },

		{ D3DXVECTOR4(-1.0f, 1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), D3DXVECTOR2( 0.66f, 0.5f ) },    // side 3 "top"
		{ D3DXVECTOR4(-1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), D3DXVECTOR2( 0.66f, 0.0f ) },
		{ D3DXVECTOR4(1.0f, 1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), D3DXVECTOR2( 1.0f, 0.5f ) },
		{ D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f), D3DXVECTOR2( 1.0f, 0.0f ) },

		{ D3DXVECTOR4(-1.0f, -1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), D3DXVECTOR2( 0.0f, 1.0f ) },    // side 4 "bottom"
		{ D3DXVECTOR4(1.0f, -1.0f, -1.0f, 1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), D3DXVECTOR2( 0.0f, 0.5f ) },
		{ D3DXVECTOR4(-1.0f, -1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), D3DXVECTOR2( 0.33f, 1.0f ) },
		{ D3DXVECTOR4(1.0f, -1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), D3DXVECTOR2( 0.33f, 0.5f ) },

		{ D3DXVECTOR4(1.0f, -1.0f, -1.0f, 1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR2( 0.66f, 1.0f ) },    // side 5 "right"
		{ D3DXVECTOR4(1.0f, 1.0f, -1.0f, 1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR2( 0.66f, 0.5f ) },
		{ D3DXVECTOR4(1.0f, -1.0f, 1.0f, 1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR2( 1.0f, 1.0f )},
		{ D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXVECTOR2( 1.0f, 0.5f ) },

		{ D3DXVECTOR4(-1.0f, -1.0f, -1.0f, 1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), D3DXVECTOR2( 0.33f, 1.0f ) },    // side 6 "left"
		{ D3DXVECTOR4(-1.0f, -1.0f, 1.0f, 1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), D3DXVECTOR2( 0.33f, 0.5f ) },
		{ D3DXVECTOR4(-1.0f, 1.0f, -1.0f, 1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), D3DXVECTOR2( 0.66f, 1.0f ) },
		{ D3DXVECTOR4(-1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), D3DXVECTOR2( 0.66f, 0.5f ) },
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

	ZeroMemory(&bd, sizeof(bd));

	myModel = (Model *)malloc(sizeof(Model));
	int res1 = myModel->modelInit("pawn.obj");
	if (!res1){

		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(VERTEX)* myModel->getSize();
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		cowVerts = myModel->getSize();

		dev->CreateBuffer(&bd, NULL, &pModelBuffer);

		devcon->Map(pModelBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, myModel->vertices->_Myfirst, bd.ByteWidth);
		devcon->Unmap(pModelBuffer, NULL);
	}

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
	dev->CreateRasterizerState(&wfdesc, &DisableCull);
	

	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_SOLID;
	wfdesc.CullMode = D3D11_CULL_BACK;
	dev->CreateRasterizerState(&wfdesc, &EnableCull);


	//set the depth stencil state.

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test params
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// If Pixel is front-facing...
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	//if pixel is back-facing...

	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;


	// Now after describing the state, we want to create and bind it to the device context.

	
	HRESULT res = dev->CreateDepthStencilState(&dsDesc, &pDSState);

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


	devcon->OMGetDepthStencilState(&pDefaultState, 0);
	devcon->OMSetDepthStencilState(pDSState, 0);

	float blendfactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	UINT sampleMask = 0xffffffff;

	devcon->OMSetBlendState( g_pBlendState, blendfactor, sampleMask);



}
// this function loads and prepares the shaders
void InitPipeline(void)
{
	// compile the shaders
	ID3D10Blob *VS, *PS, *VErrors, *PErrors, *PS2, *PSErrors2, *VS2, *VErrors2;
	ID3D10Blob *BVS, *BPS, *BPS2, *BVErrors, *BPErrors, *BPErrors2, *PSO, *PSOErrors;
	HRESULT Result;


	Result = D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "VShader", "vs_5_0", 0, 0, 0, &VS, &VErrors, 0);
	if (Result)
	{
		char *buff = (char *)VErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Vertex Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}
	Result = D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "VShader2", "vs_5_0", 0, 0, 0, &VS2, &VErrors2, 0);
	if (Result)
	{
		char *buff = (char *)VErrors2->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Vertex Shader 2 Error!", MB_OK);
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

	Result = D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "PShader2", "ps_5_0", 0, 0, 0, &PS2, &PSErrors2, 0);
	if (Result)
	{
		char *buff = (char *)PSErrors2->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Pixel Shader 2 Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	Result = D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "POShader", "ps_5_0", 0, 0, 0, &PSO, &PSOErrors, 0);
	if (Result)
	{
		char *buff = (char*)PSOErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Pixel Ordering Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	Result = D3DX11CompileFromFile(L"BadShader.hlsl", 0, 0, "VShader", "vs_5_0", 0, 0, 0, &BVS, &BVErrors, 0);
	if (Result)
	{
		char *buff = (char *)BVErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Vertex Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	Result = D3DX11CompileFromFile(L"BadShader.hlsl", 0, 0, "PShader", "ps_5_0", 0, 0, 0, &BPS, &BPErrors, 0);
	if (Result)
	{
		char *buff = (char *)BPErrors->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Pixel Shader Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	Result = D3DX11CompileFromFile(L"BadShader.hlsl", 0, 0, "PShader2", "ps_5_0", 0, 0, 0, &BPS2, &BPErrors2, 0);
	if (Result)
	{
		char *buff = (char *)BPErrors2->GetBufferPointer();
		wchar_t wtext[1000], wtext2[1000];
		mbstowcs(wtext, buff, strlen(buff) + 1);
		LPCWSTR myString = wtext;
		MessageBox(HWND_DESKTOP, myString, L"Pixel Shader 2 Error!", MB_OK);
		exit(EXIT_FAILURE);
	}
	// create the shader objects
	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreateVertexShader(VS2->GetBufferPointer(), VS2->GetBufferSize(), NULL, &pVS2);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);
	dev->CreatePixelShader(PS2->GetBufferPointer(), PS2->GetBufferSize(), NULL, &pPS2);
	dev->CreateVertexShader(BVS->GetBufferPointer(), BVS->GetBufferSize(), NULL, &pBVS);
	dev->CreatePixelShader(BPS->GetBufferPointer(), BPS->GetBufferSize(), NULL, &pBPS);
	dev->CreatePixelShader(BPS2->GetBufferPointer(), BPS2->GetBufferSize(), NULL, &pBPS2);
	dev->CreatePixelShader(PSO->GetBufferPointer(), PSO->GetBufferSize(), NULL, &pPSO);
	// set the shader objects
	

	// create the input element object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	HRESULT OK = dev->CreateInputLayout(ied, 3, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
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

void BadRenderFrame(void)
{
	CBUFFER cBuffer;

	cBuffer.LightVector = Light;
	cBuffer.LightColor = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
	cBuffer.AmbientColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
	cBuffer.Camera = Light;
	cBuffer.mode = displayMode;

	D3DXMATRIX matRotate, matView, matProjection;
	D3DXMATRIX matFinal;

	const float fClear[4] = { -1.f, -1.f, -1.f, -1.f };
	const float dfClear[4] = { 100.f, 100.f, 100.f, 100.f };
	devcon->ClearUnorderedAccessViewFloat(pUAV[0], dfClear);
	devcon->ClearUnorderedAccessViewFloat(pUAV[1], dfClear);

	devcon->ClearUnorderedAccessViewFloat(pUAV[2], fClear);
	devcon->ClearUnorderedAccessViewFloat(pUAV[3], fClear);
	static float Time = 0.0f;

	if (rotate) Time += 0.004;

	//Begin First Pass

	devcon->VSSetShader(pBVS, 0, 0);
	devcon->PSSetShader(pBPS, 0, 0);

	ID3D11RenderTargetView *pNullRTView[] = { NULL };

	//devcon->OMSetRenderTargets(1, pNullRTView, zbuffer);

	devcon->OMSetRenderTargetsAndUnorderedAccessViews(1, &RTVs[0], NULL, 1, 4, pUAV, 0);

	devcon->RSSetState(DisableCull);
	devcon->OMSetDepthStencilState(pDSState, 1);

	// create a world matrices
	D3DXMatrixRotationY(&matRotate, Time);

	// create a view matrix
	D3DXMatrixLookAtLH(&matView,
		&D3DXVECTOR3(Light.x, Light.y, Light.z),   // the camera position // - pass 1: change name of "Camera" to "Light"
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

	// create a projection matrix
	D3DXMatrixOrthoLH(&matProjection, 3.4, 3.0, 0, 1);
	/*D3DXMatrixPerspectiveFovLH(&matProjection,
	(FLOAT)D3DXToRadian(90),                    // field of view
	(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_WIDTH, // aspect ratio
	1.0f,                                       // near view-plane
	100.0f);                                    // far view-plane
	*/
	// load the matrices into the constant buffer
	cBuffer.Final = matRotate * matView * matProjection;
	cBuffer.Rotation = matRotate;
	cBuffer.modelView = matView;

	// clear the back buffer to a deep blue
	devcon->ClearRenderTargetView(RTVs[0], D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

	// clear the depth buffer
	devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// select which vertex buffer to display
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;

	//Default object use 
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
	devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);

	//devcon->IASetVertexBuffers(0, 1, &pModelBuffer, &stride, &offset);

	// select which primtive type we are using
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// draw the Hypercraft
	devcon->UpdateSubresource(pCBuffer, 0, 0, &cBuffer, 0, 0);
	devcon->DrawIndexed(36, 0, 0); //this is for the default cube object
	//devcon->Draw(cowVerts, 0);


	//end of first pass.

	if ((displayMode & MODE_PERSP_SHOW_FLAT_SCALE)){
		//begin second pass.

		//devcon->OMSetRenderTargetsAndUnorderedAccessViews(1, RTVs, zbuffer, 1, 4, nUAV, 0);

		D3DXMatrixLookAtLH(&matView,
			&D3DXVECTOR3(-4.0f, 5.0f, -5.0f),   // the camera position
			&D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
			&D3DXVECTOR3(0.0f, 1.0f, 0.0f));   // the up direction

		// create a projection matrix
		//D3DXMatrixOrthoLH(&matProjection, 4, 4, 0, 1);
		// create a projection matrix
		D3DXMatrixPerspectiveFovLH(&matProjection,
			(FLOAT)D3DXToRadian(45),                    // field of view
			(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
			1.0f,                                       // near view-plane
			100.0f);

		// load the matrices into the constant buffer
		cBuffer.Final = matRotate * matView * matProjection;
		cBuffer.Rotation = matRotate;
		cBuffer.modelView = matView;

		devcon->OMSetRenderTargetsAndUnorderedAccessViews(1, &RTVs[0], zbuffer, 1, 4, pUAV, 0);

		devcon->RSSetState(EnableCull);
		devcon->OMSetDepthStencilState(pDefaultState, 1);
		devcon->ClearRenderTargetView(RTVs[0], D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));
		devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);

		devcon->PSSetShader(pBPS2, 0, 0);

		devcon->UpdateSubresource(pCBuffer, 0, 0, &cBuffer, 0, 0);
		devcon->DrawIndexed(36, 0, 0);
		//devcon->Draw(cowVerts, 0);
	}

	// switch the back buffer and the front buffer
	swapchain->Present(0, 0);
}

void InitializeUAVs(void)
{
	//create UAV texture.  If you want the texture to be a part of a UAV resource, it MUST look like this.

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = SCREEN_WIDTH;
	texDesc.Height = SCREEN_HEIGHT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;

	HRESULT texRes = dev->CreateTexture2D(&texDesc, NULL, &pUAVTex);
	if (texRes != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Texture Creation Unsuccessful!", L"Texture Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	//create UAV for the pixel shaders to use

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVdesc;
	ZeroMemory(&UAVdesc, sizeof(UAVdesc));

	UAVdesc.Format = DXGI_FORMAT_R32_FLOAT;
	UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVdesc.Texture2D.MipSlice = 0;

	HRESULT UAVRes = dev->CreateUnorderedAccessView(pUAVTex, &UAVdesc, &pUAV[1]);
	if (UAVRes != S_OK){
		MessageBox(HWND_DESKTOP, L"Our UAV view was not successful...", L"UAV Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	//Add float depth stuff here.
	D3D11_TEXTURE2D_DESC texDesc1;
	ZeroMemory(&texDesc1, sizeof(texDesc1));
	texDesc1.Width = SCREEN_WIDTH;
	texDesc1.Height = SCREEN_HEIGHT;
	texDesc1.MipLevels = 1;
	texDesc1.ArraySize = 1;
	texDesc1.SampleDesc.Count = 1;
	texDesc1.SampleDesc.Quality = 0;
	texDesc1.Usage = D3D11_USAGE_DEFAULT;
	texDesc1.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	texDesc1.Format = DXGI_FORMAT_R32_FLOAT;
	texRes = dev->CreateTexture2D(&texDesc1, NULL, &pUAVDTex);

	if (texRes != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Depth Texture Creation Unsuccessful!", L"Texture Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	ZeroMemory(&UAVdesc, sizeof(UAVdesc));

	UAVdesc.Format = DXGI_FORMAT_R32_FLOAT;
	UAVdesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVdesc.Texture2D.MipSlice = 0;

	UAVRes = dev->CreateUnorderedAccessView(pUAVDTex, &UAVdesc, &pUAV[0]);
	if (UAVRes != S_OK){
		MessageBox(HWND_DESKTOP, L"Our UAV view was not successful...", L"UAV Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	//Add in-object float depth stuff here.

	D3D11_TEXTURE2D_DESC texDesc2;
	ZeroMemory(&texDesc2, sizeof(texDesc2));
	texDesc2.Width = SCREEN_WIDTH;
	texDesc2.Height = SCREEN_HEIGHT;
	texDesc2.MipLevels = 1;
	texDesc2.ArraySize = 1;
	texDesc2.SampleDesc.Count = 1;
	texDesc2.SampleDesc.Quality = 0;
	texDesc2.Usage = D3D11_USAGE_DEFAULT;
	texDesc2.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	texDesc2.Format = DXGI_FORMAT_R32_FLOAT;
	texRes = dev->CreateTexture2D(&texDesc2, NULL, &pUAVDTex2);

	if (texRes != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Depth Texture Creation Unsuccessful!", L"Texture Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc2;

	UAVDesc2.Format = DXGI_FORMAT_R32_FLOAT;
	UAVDesc2.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVDesc2.Texture2D.MipSlice = 0;

	UAVRes = dev->CreateUnorderedAccessView(pUAVDTex2, &UAVDesc2, &pUAV[2]);

	if (UAVRes != S_OK){
		MessageBox(HWND_DESKTOP, L"Our UAV view was not successful...", L"UAV Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	//add last UAV texture here.

	texRes = dev->CreateTexture2D(&texDesc2, NULL, &pUAVDTex3);

	if (texRes != S_OK)
	{
		MessageBox(HWND_DESKTOP, L"Depth Texture Creation Unsuccessful!", L"Texture Error!", MB_OK);
		exit(EXIT_FAILURE);
	}

	UAVRes = dev->CreateUnorderedAccessView(pUAVDTex3, &UAVDesc2, &pUAV[3]);

	if (UAVRes != S_OK){
		MessageBox(HWND_DESKTOP, L"Our UAV view was not successful...", L"UAV Error!", MB_OK);
		exit(EXIT_FAILURE);
	}
}