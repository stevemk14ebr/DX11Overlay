#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }
#include "shader_fx.h"



#ifndef UNICODE  
typedef std::string String; 
#else
typedef std::wstring String; 
#endif

#ifdef _UNICODE
#define _T(x) x
#else 
#define _T(x)  L##x
#endif

#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                              \
{                                                          \
	HRESULT hr = (x);                                      \
	if(FAILED(hr))                                         \
{                                                      \
	DXTraceW(__FILE__, (DWORD)__LINE__, hr, _T(#x), true); \
}                                                      \
}
#endif
#else
#ifndef HR
#define HR(x) (x)
#endif
#endif 


struct Vertex
{
	XMFLOAT3 Position;
	XMCOLOR Color;
};

class DXOverlay
{
public:
	DXOverlay(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa);
	bool MakeWindow();
	int RunOverlay();
	void SetToTarget();
	virtual ~DXOverlay();
	bool InitializeDX();
	virtual void OnResize();
	virtual void DrawScene()=0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DrawLine(D3DXVECTOR2 start,D3DXVECTOR2 end,float color[4]);
	void DrawRect(D3DXVECTOR2 lowerleft,D3DXVECTOR2 upperright,float color[4]);
	void DrawCircle(D3DXVECTOR2 center,float radius,int samples,float color[4]);
	void DrawText(D3DXVECTOR2 position,const char* format,...);
	void DrawTexture(D3DXVECTOR2 position);
protected:
	String WindowTitle;
	UINT m_width;
	UINT m_height;
	HINSTANCE m_appInst;
	HWND m_MainWndHandle;
	String m_targetWindowName;
	HWND m_targetApplication;
	bool m_Paused;
	bool m_Minimized;
	bool m_Resizing;
	UINT m_MSAAQualtiy;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pImmediateDeviceContext;
	ID3D11RenderTargetView* m_pRenderTargetView;
	IDXGISwapChain* m_pSwapChain;
	D3D_DRIVER_TYPE m_DriverType;
	D3D11_VIEWPORT m_Viewport;
	D3D11_BLEND_DESC m_BlendStateDesc;
	ID3D11BlendState* m_pBlendState;
	D3D11_BUFFER_DESC m_BufferDesc;
	ID3D11Buffer* m_pVertexBuffer;
	ID3D10Blob* m_ShaderFX;
	ID3DX11Effect* m_pShaderEffect;
	ID3DX11EffectTechnique* m_pTechnique;
	ID3D11InputLayout* m_pInputLayout;
};
namespace
{
	DXOverlay* DXInst=0;
}
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return DXInst->MsgProc(hwnd, msg, wParam, lParam);
}
bool DXOverlay::MakeWindow()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= m_appInst;
	wcex.hIcon			= LoadIcon(0, IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
	wcex.lpszMenuName	= WindowTitle.c_str();
	wcex.lpszClassName	= WindowTitle.c_str();
	wcex.hIconSm		= LoadIcon(0, IDI_APPLICATION);

	
	if( !RegisterClassEx(&wcex) )
	{
		return false;
	}
//
	m_MainWndHandle = CreateWindowEx(WS_EX_TOPMOST| WS_EX_LAYERED  , WindowTitle.c_str(), WindowTitle.c_str(),  WS_POPUP, 1, 1, m_width, m_height, 0, 0, 0, 0);
	
	SetLayeredWindowAttributes(m_MainWndHandle, 0, 0.0f, LWA_ALPHA);
	SetLayeredWindowAttributes(m_MainWndHandle, 0, RGB(0, 0, 0), LWA_COLORKEY);
	ShowWindow( m_MainWndHandle, SW_SHOW);
	
	const MARGINS Margin = { -1 };
	HR(DwmExtendFrameIntoClientArea(m_MainWndHandle,&Margin));

	return true;
}
bool DXOverlay::InitializeDX()
{
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	m_DriverType=D3D_DRIVER_TYPE_HARDWARE;
	HRESULT hr = D3D11CreateDevice(
		0,                 // default adapter
		m_DriverType,
		0,                 // no software device
		createDeviceFlags, 
		0, 0,              // default feature level array
		D3D11_SDK_VERSION,
		&m_pDevice,
		&featureLevel,
		&m_pImmediateDeviceContext);

	if(FAILED(hr))
		return false;

	if(featureLevel!=D3D_FEATURE_LEVEL_11_0)
		return false;

	HR(m_pDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM,4,&m_MSAAQualtiy));

	assert(m_MSAAQualtiy>0);

	DXGI_SWAP_CHAIN_DESC swapdesc;
	swapdesc.BufferDesc.Width  = m_width;
	swapdesc.BufferDesc.Height = m_height;
	swapdesc.BufferDesc.RefreshRate.Numerator = 75;
	swapdesc.BufferDesc.RefreshRate.Denominator = 1;
	swapdesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapdesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapdesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swapdesc.SampleDesc.Count=4;
	swapdesc.SampleDesc.Quality=m_MSAAQualtiy-1;

	swapdesc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapdesc.BufferCount  = 1;
	swapdesc.OutputWindow = m_MainWndHandle;
	swapdesc.Windowed     = true;
	swapdesc.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	swapdesc.Flags        = 0;

	IDXGIDevice* dxgiDevice = 0;
	HR(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	HR(dxgiFactory->CreateSwapChain(m_pDevice, &swapdesc, &m_pSwapChain));

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	ZeroMemory(&m_BlendStateDesc, sizeof(D3D11_BLEND_DESC)); 

	// Create an alpha enabled blend state description. 
	m_BlendStateDesc.RenderTarget[0].BlendEnable = TRUE; 
	m_BlendStateDesc.RenderTarget[0].SrcBlend =D3D11_BLEND_SRC_ALPHA; 
	m_BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA; 
	m_BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; 
	m_BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO; 
	m_BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; 
	m_BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD; 
	m_BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f; 
	HR(m_pDevice->CreateBlendState(&m_BlendStateDesc,&m_pBlendState));

	m_BufferDesc.Usage            = D3D11_USAGE_DYNAMIC; 
	m_BufferDesc.ByteWidth        = 31 * sizeof(Vertex); 
	m_BufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER; 
	m_BufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE; 
	m_BufferDesc.MiscFlags        = 0; 
	HR(m_pDevice->CreateBuffer(&m_BufferDesc,NULL,&m_pVertexBuffer));

	m_ShaderFX=NULL;
	ID3D10Blob* pError=NULL;
	HR(D3DX11CompileFromMemory(shaderRaw,strlen(shaderRaw),"FillTechFx",NULL,NULL,"FillTech","fx_5_0",NULL,NULL,NULL,&m_ShaderFX,&pError,NULL));
	HR(D3DX11CreateEffectFromMemory(m_ShaderFX->GetBufferPointer(),m_ShaderFX->GetBufferSize(),0,m_pDevice,&m_pShaderEffect));
	ReleaseCOM(m_ShaderFX);

	m_pTechnique=m_pShaderEffect->GetTechniqueByName("FillTech");
	
	D3D11_INPUT_ELEMENT_DESC lineLayout[] = 
	{ 
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },   
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 } 
	}; 

	D3DX11_PASS_DESC passDesc;
	HR(m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc));
	HR(m_pDevice->CreateInputLayout(lineLayout,sizeof(lineLayout)/sizeof(lineLayout[0]),passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayout));
	OnResize();
}
void DXOverlay::OnResize()
{
	assert(m_pImmediateDeviceContext);
	assert(m_pDevice);
	assert(m_pSwapChain);

	ReleaseCOM(m_pRenderTargetView);
	
	HR(m_pSwapChain->ResizeBuffers(1, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(m_pDevice->CreateRenderTargetView(backBuffer, 0, &m_pRenderTargetView));
	ReleaseCOM(backBuffer);

	m_pImmediateDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView,NULL);

	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width    = static_cast<float>(m_width/2);
	m_Viewport.Height   = static_cast<float>(m_height/2);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_pImmediateDeviceContext->RSSetViewports(1, &m_Viewport);
	
}
int DXOverlay::RunOverlay()
{
	MSG msg = {0};
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}else{	
			SetToTarget();
			DrawScene();
		}
	}
	return (int)msg.wParam;
}
void DXOverlay::SetToTarget()
{
	m_targetApplication = FindWindow(0, m_targetWindowName.c_str());
	if (m_targetApplication)
	{
		RECT tClient,tSize;
		GetClientRect(m_targetApplication,&tClient);
		GetWindowRect(m_targetApplication, &tSize);

		DWORD dwStyle = GetWindowLong(m_targetApplication, GWL_STYLE);
		int windowheight;
		int windowwidth;
		int clientheight;
		int clientwidth;
		int borderheight;
		int borderwidth;
		if(dwStyle & WS_BORDER)
		{	
			windowwidth=tSize.right-tSize.left;
			windowheight=tSize.bottom-tSize.top;

			clientwidth=tClient.right-tClient.left;
			clientheight=tClient.bottom-tClient.top;

			borderheight=(windowheight-tClient.bottom);
			borderwidth=(windowwidth-tClient.right)/2; //only want one side
			borderheight-=borderwidth; //remove bottom

			tSize.left+=borderwidth;
			tSize.top+=borderheight;
		}
		m_width=clientwidth;
		m_height=clientheight;
		MoveWindow(m_MainWndHandle, tSize.left, tSize.top,clientwidth , clientheight, true);
	}
}
LRESULT DXOverlay::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	switch( msg )
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		return 0;  
	case WM_SIZE:
		// Save the new client area dimensions.
		
		if( m_pDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				m_Minimized = true;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				m_Minimized = false;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				if( m_Minimized )
				{
					m_Minimized = false;
					OnResize();
				}else if( m_Resizing ){
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_Resizing  = true;
		return 0;

	case WM_EXITSIZEMOVE:
		m_Resizing  = false;
		OnResize();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
DXOverlay::DXOverlay(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa)
{
	WindowTitle=title;
	m_width=width;
	m_height=height;
	m_appInst=inst;
	DXInst=this;//for msgproc
	m_Paused=false;
	m_Minimized=false;
	m_Resizing=false;
	m_MSAAQualtiy=msaa;
	m_targetWindowName=target;
	ZeroMemory(&m_Viewport, sizeof(D3D11_VIEWPORT));
}
DXOverlay::~DXOverlay()
{
	ReleaseCOM(m_pRenderTargetView);
	ReleaseCOM(m_pDevice);
	if(m_pImmediateDeviceContext)
		m_pImmediateDeviceContext->ClearState();
	ReleaseCOM(m_pImmediateDeviceContext);
	ReleaseCOM(m_pSwapChain);
}
void DXOverlay::DrawLine(D3DXVECTOR2 start,D3DXVECTOR2 end,float color[4])
{
	D3D11_VIEWPORT vp; 
	UINT vpnum=1;
	m_pImmediateDeviceContext->RSGetViewports(&vpnum,&vp);

	float x0 = 2.0f * ( start.x - 0.5f ) / vp.Width - 1.0f; 
	float y0 = 1.0f - 2.0f * ( start.y - 0.5f ) / vp.Height; 
	float x1 = 2.0f * ( end.x - 0.5f ) / vp.Width - 1.0f; 
	float y1 = 1.0f - 2.0f * ( end.y - 0.5f ) / vp.Height; 

	D3D11_MAPPED_SUBRESOURCE mapData;
	Vertex* pVertex;

	HR(m_pImmediateDeviceContext->Map(m_pVertexBuffer,NULL,D3D11_MAP_WRITE_DISCARD,NULL,&mapData));
	pVertex=(Vertex*)mapData.pData;

	pVertex[0].Position.x=x0;
	pVertex[0].Position.y=y0;
	pVertex[0].Position.z=0;
	pVertex[0].Color.r=color[0]/255.0f;
	pVertex[0].Color.g=color[1]/255.0f;
	pVertex[0].Color.b=color[2]/255.0f;
	pVertex[0].Color.a=color[3]/255.0f;

	pVertex[1].Position.x=x1;
	pVertex[1].Position.y=y1;
	pVertex[1].Position.z=0;
	pVertex[1].Color.r=color[0]/255.0f;
	pVertex[1].Color.g=color[1]/255.0f;
	pVertex[1].Color.b=color[2]/255.0f;
	pVertex[1].Color.a=color[3]/255.0f;

	m_pImmediateDeviceContext->Unmap(m_pVertexBuffer,NULL);
	m_pImmediateDeviceContext->IASetInputLayout(m_pInputLayout);
	UINT Stride=sizeof(Vertex);
	UINT Offset=0;

	m_pImmediateDeviceContext->IASetVertexBuffers(0,1,&m_pVertexBuffer,&Stride,&Offset);
	m_pImmediateDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	D3DX11_TECHNIQUE_DESC tech;
	HR(m_pTechnique->GetDesc(&tech));
	for(int i=0;i<tech.Passes;i++)
	{
		m_pTechnique->GetPassByIndex(i)->Apply(0,m_pImmediateDeviceContext);
		m_pImmediateDeviceContext->Draw(2,0);
	}
}

void DXOverlay::DrawRect(D3DXVECTOR2 lowerleft,D3DXVECTOR2 upperright,float color[4])
{

}
void DXOverlay::DrawCircle(D3DXVECTOR2 center,float radius,int samples,float color[4])
{

}
void DXOverlay::DrawTexture(D3DXVECTOR2 position)
{
	
}