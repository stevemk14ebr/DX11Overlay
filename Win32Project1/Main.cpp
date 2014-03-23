// Win32Project1.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Includes.h"
#include "OverlayClasses.h"

class D3DApp:public DXOverlay
{
public:
	D3DApp(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa);
	~D3DApp();
	void DrawScene(); 
};

D3DApp::D3DApp(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa):DXOverlay(title,width,height,inst,target,msaa)
{
	//just call base class constructor
}
D3DApp::~D3DApp()
{

}
void D3DApp::DrawScene()
{
	assert(m_pImmediateDeviceContext);
	assert(m_pSwapChain);

	float clearColor[4]={1.0f,0.0f,0.0f,0.2f};
	float blend[4]={0};
		
	m_pImmediateDeviceContext->ClearRenderTargetView(m_pRenderTargetView, reinterpret_cast<const float*>(&clearColor));
	m_pImmediateDeviceContext->OMSetBlendState( m_pBlendState, blend, 0xffffffff ); 

	int colors[4]={0,0,1,1};
	DrawLine(D3DXVECTOR2(0,10),D3DXVECTOR2(100,100),colors);
	HR(m_pSwapChain->Present(0, 0));
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	D3DApp app("Overlay",600,600,hInstance,"sup - Notepad",4);
	app.MakeWindow();
	app.InitializeDX();
	app.SetToTarget();
	return app.RunOverlay();
}



