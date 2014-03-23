#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <d3dx11.h>
#include <xnamath.h>
#include <dxerr.h>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <windowsx.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#include <d3dx11effect.h>
#include <D3DX10math.h>
#include <d3d9types.h>

#pragma comment(lib, "Effects11.lib");
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "DXErr.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib,"d3dcompiler.lib")