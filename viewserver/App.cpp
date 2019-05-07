/*
 * Space simulator
 * Final project of class CST325
 * Author: Theo Lepage (tlepage@csumb.edu)

 * All required features are implemented.
 * Additional features: 
 * -- Add another 3D object (planet earth)
 * -- Add Billboards (“asteroids”)
 * -- Add a 2D object (>= 6 triangles) + color gradient

 * TO-DO:
 * Framework (Window, Camera, Light, Texture, Billboard, 2D + 3D objects...)
*/


/*
 * Structure of App.cpp
 * 1. InitDevice()
 * 2. Render()
 * 3. wWinMain()
 * 4. WndProc()
 */


#include "App.h"


// DirectX

HRESULT						CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
LRESULT						CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HINSTANCE					hInst = NULL;
HWND						hMain = NULL;

D3D_DRIVER_TYPE				g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL			g_featureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*				g_pd3dDevice = NULL;
ID3D11DeviceContext*		g_pImmediateContext = NULL;

IDXGISwapChain*				g_pSwapChain = NULL;
ID3D11RenderTargetView*		g_pRenderTargetView = NULL;

ID3D11InputLayout*			g_pVertexLayout = NULL;

ID3D11Buffer*				g_pVertexBufferBillboard = NULL;
ID3D11Buffer*				g_pVertexBufferShape = NULL;
ID3D11Buffer*				g_pVertexBufferPlane = NULL;
ID3D11Buffer*				g_pVertexBufferSky = NULL;
ID3D11Buffer*				g_pVertexBufferSpaceship = NULL;
ID3D11Buffer*				g_pVertexBufferPlanet = NULL;
ID3D11Buffer*				g_pVertexBufferMoon = NULL;
ID3D11Buffer*				g_pVertexBufferCube = NULL;
int							g_vertices_sky;
int							g_vertices_spaceship;
int							g_vertices_planet;
int							g_vertices_moon;
int							g_vertices_cube;

ID3D11Buffer*				g_pConstantBuffer = NULL;

struct CONSTANT_BUFFER
{
	float div_tex_x;
	float div_tex_y;
	float slice_x;
	float slice_y;

	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;

	XMFLOAT4 vLightDir[1];
	XMFLOAT4 vLightColor[1];
};

CONSTANT_BUFFER ConstantBuffer;

ID3D11VertexShader*			g_pVertexShader = NULL;
ID3D11PixelShader*			g_pPixelShaderLight = NULL;
ID3D11PixelShader*			g_pPixelShaderTexture = NULL;
ID3D11PixelShader*			g_pPixelShaderColor = NULL;
ID3D11PixelShader*			g_pPixelShaderGradient = NULL;

ID3D11Texture2D*            g_pDepthStencil = NULL;
ID3D11DepthStencilView*     g_pDepthStencilView = NULL;

ID3D11BlendState*			g_BlendState;

ID3D11ShaderResourceView*	g_TextureSky = NULL;
ID3D11ShaderResourceView*   g_TextureSpaceship = NULL;
ID3D11ShaderResourceView*   g_TexturePlanet = NULL;
ID3D11ShaderResourceView*   g_TexturePlanet2 = NULL;
ID3D11ShaderResourceView*   g_TextureMoon = NULL;
ID3D11ShaderResourceView*   g_TextureAsteroid1 = NULL;
ID3D11ShaderResourceView*   g_TextureAsteroid2 = NULL;

ID3D11SamplerState*         g_Sampler = NULL;

ID3D11RasterizerState		*rs_CW, *rs_CCW, *rs_NO, *rs_Wire;

ID3D11DepthStencilState		*ds_on, *ds_off;

// App

UINT		width;
UINT		height;

XMMATRIX	g_world;
XMMATRIX	g_spaceship;
XMMATRIX	g_spaceshipT;
XMMATRIX	g_view;
XMMATRIX	g_projection;

bool		wPressed = false;
bool		sPressed = false;
bool		aPressed = false;
bool		dPressed = false;
bool		upPressed = false;
bool		downPressed = false;
bool		leftPressed = false;
bool		rightPressed = false;

bool		downscale = false;

float		mouseX = 0;

struct Billboard
{
	XMFLOAT3 pos;
	ID3D11ShaderResourceView* texture;
};
int			numberOfBillboards = 6;
Billboard	billboards[6];




void GeneratePolygonVertices(SimpleVertex* vertices, int sides, float radius)
{
	float PI = 3.14159265358979323846;
	int index = 0;
	float theta = PI / sides;

	XMFLOAT3 last = XMFLOAT3(radius * cos(theta), radius * sin(theta), 0.5f);

	for (int n = 1; n <= sides; n += 1)
	{
		float x = radius * cos(theta + 2 * PI * n / sides);
		float y = radius * sin(theta + 2 * PI * n / sides);

		SimpleVertex a = SimpleVertex();
		SimpleVertex b = SimpleVertex();
		SimpleVertex c = SimpleVertex();

		a.Pos = XMFLOAT3(x, y, 0.5f);
		b.Pos = last;
		c.Pos = XMFLOAT3(0.0f, 0.0f, 0.5f);
		last = a.Pos;

		vertices[index] = a;
		vertices[index + 1] = b;
		vertices[index + 2] = c;
		index += 3;
	}
}







/*
 * InitDevice()
 * Init DirectX 3D objects
*/

HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hMain, &rc);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
	#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hMain;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VShader", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the LIGHT pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSLight", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the LIGHT pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderLight);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the TEXTURE pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSTexture", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the TEXTURE pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderTexture);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the COLOR pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSColor", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the COLOR pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderColor);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the GRADIENT pixel shader
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSGradient", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the GRADIENT pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderGradient);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Create g_pVertexBufferBillboard
	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA InitData;
	SimpleVertex vertices[6];
	vertices[0].Pos = XMFLOAT3(-1, 1, 1);	//left top
	vertices[1].Pos = XMFLOAT3(1, -1, 1);	//right bottom
	vertices[2].Pos = XMFLOAT3(-1, -1, 1); //left bottom
	vertices[0].Tex = XMFLOAT2(0.0f, 0.0f);
	vertices[1].Tex = XMFLOAT2(1.0f, 1.0f);
	vertices[2].Tex = XMFLOAT2(0.0f, 1.0f);

	vertices[3].Pos = XMFLOAT3(-1, 1, 1);	//left top
	vertices[4].Pos = XMFLOAT3(1, 1, 1);	//right top
	vertices[5].Pos = XMFLOAT3(1, -1, 1);	//right bottom
	vertices[3].Tex = XMFLOAT2(0.0f, 0.0f);			//left top
	vertices[4].Tex = XMFLOAT2(1.0f, 0.0f);			//right top
	vertices[5].Tex = XMFLOAT2(1.0f, 1.0f);			//right bottom	

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferBillboard);
	if (FAILED(hr))
		return hr;


	// Create g_pVertexBufferShape
	SimpleVertex vertices2[24];
	GeneratePolygonVertices(vertices2, 8, 1);

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 24;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices2;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferShape);
	if (FAILED(hr))
		return hr;

	// Create constant buffer
	ConstantBuffer.div_tex_x = 1;
	ConstantBuffer.div_tex_y = 1;
	ConstantBuffer.slice_x = 0;
	ConstantBuffer.slice_y = 0;

	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.ByteWidth = sizeof(CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &ConstantBuffer;
	hr = g_pd3dDevice->CreateBuffer(&cbDesc, &InitData, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	// Load textures
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/sky.jpg", NULL, NULL, &g_TextureSky, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/spaceship.jpg", NULL, NULL, &g_TextureSpaceship, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/planet.jpg", NULL, NULL, &g_TexturePlanet, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/planet2.jpg", NULL, NULL, &g_TexturePlanet2, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/moon.jpg", NULL, NULL, &g_TextureMoon, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/asteroid1.jpg", NULL, NULL, &g_TextureAsteroid1, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"textures/asteroid2.png", NULL, NULL, &g_TextureAsteroid2, NULL);
	if (FAILED(hr))
		return hr;

	// Init billboards
	int x = -12;
	int y = 3;
	for (int i = 0; i < numberOfBillboards; i++)
	{
		// Set texture
		if (i % 2 == 0)
		{
			billboards[i].texture = g_TextureAsteroid1;
			y = -3;
		}
		else
		{
			billboards[i].texture = g_TextureAsteroid2;
			y = 3;
		}

		// Set position
		billboards[i].pos = XMFLOAT3(x, y, -10);
		x += 6;
	}



	// Sampler init
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_Sampler);
	if (FAILED(hr))
		return hr;

	// Blendstate
	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);

	// Init matrices

	g_spaceshipT = XMMatrixTranslation(-3, -1, -12);

	XMVECTOR Eye = XMVectorSet(0, 0, -20, 0.0f); //camera position
	XMVECTOR At = XMVectorSet(0, 0, 1, 0.0f); //look at
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // normal vector on at vector (always up)
	
	g_view = XMMatrixLookAtLH(Eye, At, Up);
	g_world = XMMatrixIdentity();
	g_projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 10000.0f);

	// Load models
	Load3DS("models/sphere.3ds", g_pd3dDevice, &g_pVertexBufferSky, &g_vertices_sky,FALSE);
	LoadOBJ("models/spaceship.obj", g_pd3dDevice, &g_pVertexBufferSpaceship, &g_vertices_spaceship);
	Load3DS("models/sphere.3ds", g_pd3dDevice, &g_pVertexBufferPlanet, &g_vertices_planet, FALSE);
	Load3DS("models/sphere.3ds", g_pd3dDevice, &g_pVertexBufferMoon, &g_vertices_moon, FALSE);
	Load3DS("models/cube.3ds", g_pd3dDevice, &g_pVertexBufferCube, &g_vertices_cube, FALSE);

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	// Rasterizer states:
	D3D11_RASTERIZER_DESC RS_CW, RS_Wire;
	RS_CW.AntialiasedLineEnable = FALSE;
	RS_CW.CullMode = D3D11_CULL_BACK;
	RS_CW.DepthBias = 0;
	RS_CW.DepthBiasClamp = 0.0f;
	RS_CW.DepthClipEnable = true;
	RS_CW.FillMode = D3D11_FILL_SOLID;
	RS_CW.FrontCounterClockwise = false;
	RS_CW.MultisampleEnable = FALSE;
	RS_CW.ScissorEnable = false;
	RS_CW.SlopeScaledDepthBias = 0.0f;
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CW);
	RS_CW.CullMode = D3D11_CULL_FRONT;
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CCW);
	RS_Wire = RS_CW;
	RS_Wire.CullMode = D3D11_CULL_NONE;
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_NO);
	RS_Wire.FillMode = D3D11_FILL_WIREFRAME;
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_Wire);

	// Init depth stats
	D3D11_DEPTH_STENCIL_DESC DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);

	return S_OK;
}

/*
 * Render()
 * Render function
*/
void Render()
{
	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		t = (dwTimeCur - dwTimeStart) / 1000.0f;
	}

	// Clear the back buffer 
	float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Mask of texture billboards
	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_Sampler);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_Sampler);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;





	//
	// Camera
	//

	XMMATRIX g_viewR = XMMatrixRotationY(-(2 * mouseX) / width + 1);

	if (upPressed)
	{
		g_view *= XMMatrixTranslation(0, 0, -0.01);
	}
	if (downPressed)
	{
		g_view *= XMMatrixTranslation(0, 0, 0.01);
	}
	if (leftPressed)
	{
		g_view *= XMMatrixTranslation(0.01, 0, 0);
	}
	if (rightPressed)
	{
		g_view *= XMMatrixTranslation(-0.01, 0, 0);
	}

	ConstantBuffer.view = g_view * g_viewR;
	ConstantBuffer.projection = g_projection;

	//
	// Sky
	//

	XMMATRIX Tv = XMMatrixTranslation(0, 0, 2);
	XMMATRIX Rx = XMMatrixRotationX(XM_PIDIV2);
	XMMATRIX S = XMMatrixScaling(0.2, 0.2, 0.2);
	XMMATRIX Msky = S*Rx*Tv; //from left to right
	ConstantBuffer.world = Msky;

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderTexture, NULL, 0);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	
	// Set model and texture
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferSky, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureSky);
	
	g_pImmediateContext->RSSetState(rs_CCW); // To see it from the inside
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1); // No depth writing
	
	g_pImmediateContext->Draw(g_vertices_sky, 0);
	
	g_pImmediateContext->RSSetState(rs_CW);
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);
	
	//
	// Spaceship
	//

	// Init spaceship
	g_spaceship = XMMatrixRotationY(XM_PI);
	if (downscale)
	{
		g_spaceship *= XMMatrixScaling(0.1, 0.1, 0.1);
	}
	else
	{
		g_spaceship *= XMMatrixScaling(0.2, 0.2, 0.2);
	}

	float spaceshipSpeed = 0.005;
	if (wPressed)
	{
		g_spaceshipT *= XMMatrixTranslation(0, 0, spaceshipSpeed);
	}
	if (sPressed)
	{
		g_spaceshipT *= XMMatrixTranslation(0, 0, -spaceshipSpeed);
	}
	if (aPressed)
	{
		g_spaceshipT *= XMMatrixTranslation(-spaceshipSpeed, 0, 0);
	}
	if (dPressed)
	{
		g_spaceshipT *= XMMatrixTranslation(spaceshipSpeed, 0, 0);
	}

	ConstantBuffer.world = g_spaceship * g_spaceshipT;
	
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureSpaceship);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferSpaceship, &stride, &offset);
	g_pImmediateContext->Draw(g_vertices_spaceship, 0);

	//
	// Planet
	//

	XMMATRIX T = XMMatrixTranslation(0, 0, 0);

	if (downscale)
	{
		S = XMMatrixScaling(0.0025, 0.0025, 0.0025);
	}
	else
	{
		S = XMMatrixScaling(0.005, 0.005, 0.005);
	}
	
	ConstantBuffer.world = XMMatrixRotationX(XM_PIDIV2) * S * T;

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TexturePlanet);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_TexturePlanet);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferPlanet, &stride, &offset);
	g_pImmediateContext->Draw(g_vertices_planet, 0);

	//
	// Moon
	//

	T = XMMatrixTranslation(0, 0, 4);
	S = XMMatrixScaling(0.001, 0.001, 0.001);
	XMMATRIX orbit = XMMatrixRotationY(t);

	if (downscale)
	{
		S = XMMatrixScaling(0.0005, 0.0005, 0.0005);
	}
	else
	{
		S = XMMatrixScaling(0.001, 0.001, 0.001);
	}

	ConstantBuffer.world = S * T * orbit;

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureMoon);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_TextureMoon);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferMoon, &stride, &offset);
	g_pImmediateContext->Draw(g_vertices_planet, 0);

	//
	// Planet2
	//

	T = XMMatrixTranslation(10, -2, -3);

	if (downscale)
	{
		S = XMMatrixScaling(0.001, 0.001, 0.001);
	}
	else
	{
		S = XMMatrixScaling(0.002, 0.002, 0.002);
	}

	ConstantBuffer.world = XMMatrixRotationX(XM_PIDIV2) * S * T;

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TexturePlanet2);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_TexturePlanet2);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferPlanet, &stride, &offset);
	g_pImmediateContext->Draw(g_vertices_planet, 0);


	//
	// Light
	//


	XMFLOAT4 vLightDirs[1] =
	{
		XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)
	};
	XMFLOAT4 vLightColors[1] =
	{
		XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
	};

	// Rotate the light around the origin
	XMVECTOR vLightDir = XMLoadFloat4(&vLightDirs[0]);
	vLightDir = XMVector3Transform(vLightDir, orbit);
	XMStoreFloat4(&vLightDirs[0], vLightDir);

	ConstantBuffer.vLightDir[0] = vLightDirs[0];
	ConstantBuffer.vLightColor[0] = vLightColors[0];


	//
	// Cube1
	//

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderLight, NULL, 0);

	T = XMMatrixTranslation(-6, 2, -4);

	if (downscale)
	{
		S = XMMatrixScaling(0.25, 0.25, 0.25);
	}
	else
	{
		S = XMMatrixScaling(0.5, 0.5, 0.5);
	}

	ConstantBuffer.world = XMMatrixRotationX(XM_PIDIV2) * S * T;

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TexturePlanet2);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_TexturePlanet2);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferCube, &stride, &offset);
	g_pImmediateContext->Draw(g_vertices_cube, 0);

	//
	// Cube2, orbiting around Cube1
	//

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderColor, NULL, 0);

	// 1. translate by distance orbit
	// 2. rotate Y
	// 3. translate to place a center of Cube1 (T2)

	T = XMMatrixTranslation(0, 0, 2);
	orbit = XMMatrixRotationY(-t);
	XMMATRIX T2 = XMMatrixTranslation(-6, 2, -4);

	if (downscale)
	{
		S = XMMatrixScaling(0.05, 0.05, 0.05);
	}
	else
	{
		S = XMMatrixScaling(0.1, 0.1, 0.1);
	}

	ConstantBuffer.world = S * T * orbit * T2;

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TexturePlanet2);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_TexturePlanet2);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferCube, &stride, &offset);
	g_pImmediateContext->Draw(g_vertices_cube, 0);


	//
	// Billboard
	//

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderTexture, NULL, 0);

	//Create the inverse rotation view matrix
	XMMATRIX Vc = g_view * g_viewR;
	Vc._41 = 0;
	Vc._42 = 0;
	Vc._43 = 0;
	XMVECTOR f;
	Vc = XMMatrixInverse(&f, Vc);

	T = XMMatrixTranslation(5, 0, 10);
	S = XMMatrixScaling(1.0, 1.0, 1.0);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferBillboard, &stride, &offset);

	for (int i = 0; i < numberOfBillboards; i++)
	{
		T = XMMatrixTranslation(billboards[i].pos.x, billboards[i].pos.y, billboards[i].pos.z);
		ConstantBuffer.world = Vc * S * T;
		g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);
		g_pImmediateContext->PSSetShaderResources(0, 1, &billboards[i].texture);
		g_pImmediateContext->Draw(6, 0);
	}



	//
	// 2D Shape
	//

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderGradient, NULL, 0);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferShape, &stride, &offset);

	T = XMMatrixTranslation(0, 3, -10);
	ConstantBuffer.world = Vc * S * T;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, 0, &ConstantBuffer, 0, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureAsteroid1);
	g_pImmediateContext->Draw(24, 0);


	g_pSwapChain->Present(0, 0);
}

/*
 * wWinMain()
 * Main function
*/

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	
	// Register window
	WNDCLASSEX wcex;
	BOOL Result = TRUE;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"AppWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, NULL);					
	Result = (RegisterClassEx(&wcex) != 0);
					
	// Open window
	hMain = CreateWindow(L"AppWindowClass", L"Space simulator", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, NULL, NULL, hInstance, NULL);
	if (hMain == 0)	return 0;
	ShowWindow(hMain, nCmdShow);
	UpdateWindow(hMain);

	// Init DirectX 3D objects
	if (FAILED(InitDevice()))
	{
		return 0;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	return (int)msg.wParam;
}

/*
 * WndProc()
 * Handle events
*/

BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	hMain = hwnd;
	return TRUE;
}

void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{

	switch (vk)
		{
			case 87: //w
				wPressed = true;
				break;

			case 83: //s
				sPressed = true;
				break;

			case 65: //a
				aPressed = true;
				break;

			case 68: //d
				dPressed = true;
				break;

			case 74: // j
				downscale = true;
				break;

			case 75: // k
				downscale = false;
				break;

			case VK_UP: //up
				upPressed = true;
				break;

			case VK_DOWN: //down
				downPressed = true;
				break;

			case VK_LEFT: //left
				leftPressed = true;
				break;

			case VK_RIGHT: //right
				rightPressed = true;
				break;
			
			default:break;

		}
}

void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
	case 87: //w
		wPressed = false;
		break;

	case 83: //s
		sPressed = false;
		break;

	case 65: //a
		aPressed = false;
		break;

	case 68: //d
		dPressed = false;
		break;

	case VK_UP: //up
		upPressed = false;
		break;

	case VK_DOWN: //down
		downPressed = false;
		break;

	case VK_LEFT: //left
		leftPressed = false;
		break;

	case VK_RIGHT: //right
		rightPressed = false;
		break;

	default:break;

	}
}

void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	mouseX = x;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
	HANDLE_MSG(hwnd, WM_KEYDOWN, OnKeyDown);
	HANDLE_MSG(hwnd, WM_KEYUP, OnKeyUp);
	HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);

	case WM_PAINT:
		hdc = BeginPaint(hMain, &ps);
		EndPaint(hMain, &ps);
		break;
	case WM_ERASEBKGND:
		return (LRESULT)1;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

/*
 * CompileShaderFromFile()
 * Helper for compiling shaders with D3DX11
*/

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
	#if defined( DEBUG ) || defined( _DEBUG )
		dwShaderFlags |= D3DCOMPILE_DEBUG;
	#endif
	ID3DBlob* pErrorBlob;

	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);

	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}