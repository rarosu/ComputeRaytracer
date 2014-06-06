//--------------------------------------------------------------------------------------
// File: TemplateMain.cpp
//
// BTH-D3D-Template
//
// Copyright (c) Stefan Petersson 2013. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"

#include <fstream>
#include "ComputeHelp.h"
#include "D3D11Timer.h"
#include "input_state.h"
#include "Camera.h"
#include "ModelLoader.h"

/*	DirectXTex library - for usage info, see http://directxtex.codeplex.com/
	
	Usage example (may not be the "correct" way, I just wrote it in a hurry):

	DirectX::ScratchImage img;
	DirectX::TexMetadata meta;
	ID3D11ShaderResourceView* srv = nullptr;
	if(SUCCEEDED(hr = DirectX::LoadFromDDSFile(_T("C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Samples\\Media\\Dwarf\\Armor.dds"), 0, &meta, img)))
	{
		//img loaded OK
		if(SUCCEEDED(hr = DirectX::CreateShaderResourceView(g_Device, img.GetImages(), img.GetImageCount(), meta, &srv)))
		{
			//srv created OK
		}
	}
*/
#include <DirectXTex.h>

#if defined( DEBUG ) || defined( _DEBUG )
#pragma comment(lib, "DirectXTexD.lib")
#else
#pragma comment(lib, "DirectXTex.lib")
#endif

//--------------------------------------------------------------------------------------
// Structure definitions
//-------------------------------------------------------------------------------------
struct sphere {
	glm::vec4 m_position; // w = radius
	glm::vec4 m_diffuse;
	glm::vec4 m_specular;
};



struct Ray {
	glm::vec4 m_origin;
	glm::vec4 m_direction;
};

const unsigned int PRIMITIVE_TYPE_NONE = 0;
const unsigned int PRIMITIVE_TYPE_SPHERE = 1;
const unsigned int PRIMITIVE_TYPE_TRIANGLE = 2;

struct HitData {
	float m_t;
	bool m_hit;
	glm::vec4 m_position;
	glm::vec4 m_normal;
	unsigned int m_primitiveType;
	unsigned int m_primitiveIndex;

	// Triangle specific.
	glm::vec2 m_barycentricCoords;
};

struct PointLight {
	glm::vec3 m_position;
	float m_radius;
	glm::vec4 m_diffuse;
	glm::vec4 m_specular;
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE				g_hInst					= NULL;  
HWND					g_hWnd					= NULL;

IDXGISwapChain*         g_SwapChain				= NULL;
ID3D11Device*			g_Device				= NULL;
ID3D11DeviceContext*	g_DeviceContext			= NULL;

ID3D11UnorderedAccessView*  g_BackBufferUAV		= NULL;  // compute output

ComputeWrap*			g_ComputeSys			= NULL;

D3D11Timer*				g_Timer					= NULL;

int g_Width, g_Height;

//--------------------------------------------------------------------------------------
// DEMO SPECIFIC GLOBALS!
//--------------------------------------------------------------------------------------
const int WIDTH = 800;
const int HEIGHT = 800;
const int THREAD_GROUPS_X = 25;
const int THREAD_GROUPS_Y = 25;

struct CameraCB
{
	glm::mat4 c_view;
	glm::mat4 c_projection;
	glm::mat4 c_inv_vp;
	glm::vec4 c_cameraPos;
};

struct OnceCB
{
	int c_windowWidth;
	int c_windowHeight;
};

input_state g_current_input, g_previous_input;
Camera* g_camera = NULL;

std::vector<sphere> g_spheres;
std::vector<Tri> g_triangles;
std::vector<PointLight> g_pointLights;

ComputeShader* g_PrimaryShader = NULL;
ComputeShader* g_IntersectionShader = NULL;
ComputeShader* g_ColoringShader = NULL;

ComputeBuffer* g_rayBuffer = NULL;
ComputeBuffer* g_hitBuffer = NULL;
ComputeBuffer* g_sphere_buffer = NULL;
ComputeBuffer* g_triangleBuffer = NULL;

CameraCB* g_cameraBufferData = NULL;
ID3D11Buffer* g_cameraBuffer = NULL;

OnceCB* g_onceBufferData = NULL;
ID3D11Buffer* g_onceBuffer = NULL;

ID3D11Buffer* g_pointLightBuffer = NULL;

ID3D11Buffer* g_aabbBuffer = NULL;

std::ofstream g_log;

float g_t = 0.0f;

//--------------------------------------------------------------------------------------
// DEMO SPECIFIC FORWARD DECLARATIONS!
//--------------------------------------------------------------------------------------

void UpdatePointLights();

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT             InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT				Render(float deltaTime);
HRESULT				Update(float deltaTime);
HRESULT				UpdateBuffer(ID3D11Buffer* p_buffer, void* p_data, unsigned int p_size);

char* FeatureLevelToString(D3D_FEATURE_LEVEL featureLevel)
{
	if(featureLevel == D3D_FEATURE_LEVEL_11_0)
		return "11.0";
	if(featureLevel == D3D_FEATURE_LEVEL_10_1)
		return "10.1";
	if(featureLevel == D3D_FEATURE_LEVEL_10_0)
		return "10.0";

	return "Unknown";
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT Init()
{
	HRESULT hr = S_OK;;

	RECT rc;
	GetClientRect( g_hWnd, &rc );
	g_Width = rc.right - rc.left;;
	g_Height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverType;

	D3D_DRIVER_TYPE driverTypes[] = 
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_Width;
	sd.BufferDesc.Height = g_Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL featureLevelsToTry[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	D3D_FEATURE_LEVEL initiatedFeatureLevel;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			driverType,
			NULL,
			createDeviceFlags,
			featureLevelsToTry,
			ARRAYSIZE(featureLevelsToTry),
			D3D11_SDK_VERSION,
			&sd,
			&g_SwapChain,
			&g_Device,
			&initiatedFeatureLevel,
			&g_DeviceContext);

		if( SUCCEEDED( hr ) )
		{
			char title[256];
			sprintf_s(
				title,
				sizeof(title),
				"BTH - Direct3D 11.0 Template | Direct3D 11.0 device initiated with Direct3D %s feature level",
				FeatureLevelToString(initiatedFeatureLevel)
			);
			SetWindowTextA(g_hWnd, title);

			break;
		}
	}
	if( FAILED(hr) )
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer;
	hr = g_SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
	if( FAILED(hr) )
		return hr;

	// create shader unordered access view on back buffer for compute shader to write into texture
	hr = g_Device->CreateUnorderedAccessView( pBackBuffer, NULL, &g_BackBufferUAV );

	//create helper sys and compute shader instance
	g_ComputeSys = new ComputeWrap(g_Device, g_DeviceContext);
	g_Timer = new D3D11Timer(g_Device, g_DeviceContext);


	//--------------------------------------------------------------------------------------
	// DEMO SPECIFIC INITIALIZATION
	//--------------------------------------------------------------------------------------

	// Open log file.
	g_log.open("log.txt", std::ios_base::trunc | std::ios_base::out);
	if (!g_log.is_open())
	{
		throw "Failed to open log file";
	}

	// Initialize input structs.
	memset(&g_current_input, 0, sizeof(input_state));
	memset(&g_previous_input, 0, sizeof(input_state));

	// Set mouse to center.
	POINT center;
	center.x = (LONG) (g_Width * 0.5f);
	center.y = (LONG) (g_Height * 0.5f);
	ClientToScreen(g_hWnd, &center);
	SetCursorPos(center.x, center.y);
	ShowCursor(FALSE);

	// Setup the shaders.
	g_PrimaryShader = g_ComputeSys->CreateComputeShader(_T("../Shaders/Primary.fx"), NULL, "main", NULL);
	g_IntersectionShader = g_ComputeSys->CreateComputeShader(_T("../Shaders/Intersection.fx"), NULL, "main", NULL);
	g_ColoringShader = g_ComputeSys->CreateComputeShader(_T("../Shaders/Coloring.fx"), NULL, "main", NULL);

	// Setup persistent data.
	g_onceBufferData = new OnceCB;
	g_onceBufferData->c_windowWidth = g_Width;
	g_onceBufferData->c_windowHeight = g_Height;

	// Setup the scene data.
	g_spheres.resize(2);
	g_spheres[0].m_position = glm::vec4(0.0f, 0.0f, 10.0f, 3.3f);
	g_spheres[0].m_diffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	g_spheres[0].m_specular = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	g_spheres[1].m_position = glm::vec4(0.0f, 0.0f, 5.0f, 1.3f);
	g_spheres[1].m_diffuse = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	g_spheres[1].m_specular = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	AABB aabb;
	AABB unused;
	LoadModel("../Models/interiorcube.obj", g_triangles, unused);
	LoadModel("../Models/shipB_OBJ.obj", g_triangles, aabb);

	g_pointLights.resize(10);
	UpdatePointLights();

	// Create a camera.
	g_camera = new Camera(Camera::CreatePerspectiveProjection(1.0f, 1000.0f, 45.0f, 1.0f));
	g_cameraBufferData = new CameraCB;
	g_cameraBufferData->c_view = g_camera->GetView();
	g_cameraBufferData->c_projection = g_camera->GetProjection();
	g_cameraBufferData->c_inv_vp = g_camera->GetInverseViewProjection();
	g_cameraBufferData->c_cameraPos = glm::vec4(g_camera->GetPosition(), 1.0f);

	// Create the buffers.
	g_cameraBuffer = g_ComputeSys->CreateDynamicBuffer(sizeof(CameraCB), (void*)g_cameraBufferData, "Camera Buffer");
	g_onceBuffer = g_ComputeSys->CreateDynamicBuffer(sizeof(OnceCB), (void*)g_onceBufferData, "Once Buffer");
	g_pointLightBuffer = g_ComputeSys->CreateDynamicBuffer((UINT)(sizeof(PointLight) * g_pointLights.size()), (void*)&g_pointLights[0], "PointLights");
	g_aabbBuffer = g_ComputeSys->CreateDynamicBuffer(sizeof(AABB), (void*)&aabb, "AABB Buffer");
	g_rayBuffer = g_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(Ray), (UINT) WIDTH * HEIGHT, true, true, 0, false, "Ray Buffer");
	g_hitBuffer = g_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(HitData), (UINT) WIDTH * HEIGHT, true, true, 0, false, "Hit Buffer");
	g_sphere_buffer = g_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(sphere), (UINT)g_spheres.size(), true, true, &g_spheres[0], false, "Spheres");
	g_triangleBuffer = g_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(Tri), (UINT)g_triangles.size(), true, true, &g_triangles[0], false, "Triangles");
	
	
	

	return S_OK;
}

HRESULT Update(float deltaTime)
{
	g_t += deltaTime;

	// Handle camera controls
	POINT center;
	center.x = (LONG) (g_Width * 0.5f);
	center.y = (LONG) (g_Height * 0.5f);

	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(g_hWnd, &pos);

	float sensitivity = 0.05f;
	float speed = 20.0f;
	float dx = (float)(center.x - pos.x);
	float dy = -(float)(center.y - pos.y);
	
	g_camera->Yaw(dx * sensitivity);
	g_camera->Pitch(dy * sensitivity);

	if (g_current_input.m_keyboard.m_keys['W'])
		g_camera->SetPosition(g_camera->GetPosition() + g_camera->GetFacing() * speed * deltaTime);
	if (g_current_input.m_keyboard.m_keys['S'])
		g_camera->SetPosition(g_camera->GetPosition() - g_camera->GetFacing() * speed * deltaTime);
	if (g_current_input.m_keyboard.m_keys['A'])
		g_camera->SetPosition(g_camera->GetPosition() - g_camera->GetRight() * speed * deltaTime);
	if (g_current_input.m_keyboard.m_keys['D'])
		g_camera->SetPosition(g_camera->GetPosition() + g_camera->GetRight() * speed * deltaTime);

	const glm::vec3& position = g_camera->GetPosition();
	const glm::vec3& facing = g_camera->GetFacing();
	glm::vec3 right = g_camera->GetRight();

	g_camera->Commit();

	g_cameraBufferData->c_view = g_camera->GetView();
	g_cameraBufferData->c_projection = g_camera->GetProjection();
	g_cameraBufferData->c_inv_vp = g_camera->GetInverseViewProjection();
	g_cameraBufferData->c_cameraPos = glm::vec4(g_camera->GetPosition(), 1.0f);
	UpdateBuffer(g_cameraBuffer, g_cameraBufferData, sizeof(CameraCB));
	
	UpdatePointLights();
	UpdateBuffer(g_pointLightBuffer, &g_pointLights[0], sizeof(PointLight) * g_pointLights.size());

	ClientToScreen(g_hWnd, &center);
	SetCursorPos(center.x, center.y);

	return S_OK;
}

HRESULT Render(float deltaTime)
{
	double primaryTime = 0.0f;
	double intersectionTime = 0.0f;
	double coloringTime = 0.0f;

	ID3D11UnorderedAccessView* uav[] = { g_BackBufferUAV, g_rayBuffer->GetUnorderedAccessView(), g_hitBuffer->GetUnorderedAccessView(), g_sphere_buffer->GetUnorderedAccessView(), g_triangleBuffer->GetUnorderedAccessView() };

	g_DeviceContext->CSSetConstantBuffers(0, 1, &g_cameraBuffer);
	g_DeviceContext->CSSetConstantBuffers(1, 1, &g_onceBuffer);
	g_DeviceContext->CSSetConstantBuffers(2, 1, &g_pointLightBuffer);
	g_DeviceContext->CSSetConstantBuffers(3, 1, &g_aabbBuffer);


	// Primary Ray Stage
	g_DeviceContext->CSSetUnorderedAccessViews(0, 5, uav, NULL);
	
	g_PrimaryShader->Set();
	g_Timer->Start();
	g_DeviceContext->Dispatch(THREAD_GROUPS_X, THREAD_GROUPS_Y, 1);
	g_Timer->Stop();
	g_PrimaryShader->Unset();
	primaryTime = g_Timer->GetTime();


	// Intersection stage
	g_IntersectionShader->Set();
	g_Timer->Start();
	g_DeviceContext->Dispatch(THREAD_GROUPS_X, THREAD_GROUPS_Y, 1);
	g_Timer->Stop();
	g_IntersectionShader->Unset();
	intersectionTime = g_Timer->GetTime();

	// Coloring stage
	g_ColoringShader->Set();
	g_Timer->Start();
	g_DeviceContext->Dispatch(THREAD_GROUPS_X, THREAD_GROUPS_Y, 1);
	g_Timer->Stop();
	g_ColoringShader->Unset();
	coloringTime = g_Timer->GetTime();


	// Presentation! :D
	if(FAILED(g_SwapChain->Present( 0, 0 )))
		return E_FAIL;


	char title[256];
	sprintf_s(
		title,
		sizeof(title),
		"BTH - DirectCompute DEMO - Dispatch time: %f",
		(primaryTime + intersectionTime + coloringTime)
	);
	SetWindowTextA(g_hWnd, title);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
		return 0;

	if( FAILED( Init() ) )
		return 0;

	__int64 cntsPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&cntsPerSec);
	float secsPerCnt = 1.0f / (float)cntsPerSec;

	__int64 prevTimeStamp = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&prevTimeStamp);

	// Main message loop
	MSG msg = {0};
	while(WM_QUIT != msg.message)
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			__int64 currTimeStamp = 0;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTimeStamp);
			float dt = (currTimeStamp - prevTimeStamp) * secsPerCnt;

			//render
			Update(dt);
			Render(dt);

			prevTimeStamp = currTimeStamp;
			g_previous_input = g_current_input;
		}
	}

	return (int) msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = 0;
	wcex.hCursor        = LoadCursor(NULL, IDC_NO);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = NULL;
	wcex.lpszClassName  = _T("BTH_D3D_Template");
	wcex.hIconSm        = 0;
	if( !RegisterClassEx(&wcex) )
		return E_FAIL;

	// Create window
	g_hInst = hInstance; 
	RECT rc = { 0, 0, WIDTH, HEIGHT };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	
	if(!(g_hWnd = CreateWindow(
							_T("BTH_D3D_Template"),
							_T("BTH - Direct3D 11.0 Template"),
							WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							rc.right - rc.left,
							rc.bottom - rc.top,
							NULL,
							NULL,
							hInstance,
							NULL)))
	{
		return E_FAIL;
	}

	ShowWindow( g_hWnd, nCmdShow );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:

		g_current_input.m_keyboard.m_keys[wParam] = true;

		break;

	case WM_KEYUP:

		g_current_input.m_keyboard.m_keys[wParam] = false;

		break;

	case WM_MOUSEMOVE:

		g_current_input.m_mouse.m_position.x = LOWORD(lParam);
		g_current_input.m_mouse.m_position.y = HIWORD(lParam);

		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

HRESULT UpdateBuffer( ID3D11Buffer* p_buffer, void* p_data, unsigned int p_size )
{
	HRESULT result = S_OK;

	D3D11_MAPPED_SUBRESOURCE map;
	result = g_DeviceContext->Map(p_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(result))
	{
		memcpy(map.pData, p_data, p_size);

		g_DeviceContext->Unmap(p_buffer, 0);
	}
	
	return result;
}

void UpdatePointLights()
{
	float radius = 30.0f;
	float angspeed = 1.5f;
	for (size_t i = 0; i < g_pointLights.size(); ++i) {
		float angle = i * 2 * 3.141592f / g_pointLights.size() + g_t * angspeed;
		g_pointLights[i].m_position = radius * glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) + glm::vec3(0.0f, 5.0f, 0.0f);
		g_pointLights[i].m_radius = radius * 2;
		g_pointLights[i].m_diffuse = i * glm::vec4(fabs(cos(angle)), fabs(cos(angle)), fabs(sin(angle)), 0.0f) / g_pointLights.size();
		g_pointLights[i].m_specular = i * glm::vec4(fabs(cos(angle)), fabs(cos(angle)), fabs(sin(angle)), 0.0f) / g_pointLights.size();
	}
}
