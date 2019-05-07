#define WIN32_LEAN_AND_MEAN  
#include <windows.h>
#include <windowsx.h>
#include <malloc.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <time.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <fstream>
using namespace std;
#include <direct.h>
#include <commdlg.h>
#include <malloc.h>
#include <cmath>
#include <string.h>
#include <tchar.h>

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>


struct SimpleVertex
{
	XMFLOAT3 Pos;//12 byte
	XMFLOAT2 Tex;//8 byte
	XMFLOAT3 Norm;//12 byte
};


bool Load3DS(char *filename, ID3D11Device* g_pd3dDevice, ID3D11Buffer **ppVertexBuffer, int *vertex_count,bool gouraurd);
bool LoadOBJ(char * filename, ID3D11Device * g_pd3dDevice, ID3D11Buffer ** ppVertexBuffer, int * vertex_count);