#include "GraphicsClass.h"

GraphicsClass::GraphicsClass()
{
	m_direct3D = 0;
}

GraphicsClass::GraphicsClass(const GraphicsClass&)
{
}

GraphicsClass::~GraphicsClass()
{
}

bool GraphicsClass::Initialize(int screenHeight, int screenWidth, HWND hwnd)
{
	bool result;

	m_direct3D = new D3DClass;
	if (!m_direct3D)
	{
		return false;
	}

	result = m_direct3D->Initialize(screenHeight, screenWidth, hwnd, VSYNC_ENABLED, FULL_SCREEN);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}
	
	return true;
}

void GraphicsClass::Shutdown()
{
	if (m_direct3D)
	{
		m_direct3D->Shutdown();
		delete m_direct3D;
		m_direct3D = 0;
	}
}

bool GraphicsClass::Frame()
{
	bool result;

	result = Render();
	if (!result)
	{
		return false;
	}
	
	return true;
}

bool GraphicsClass::Render()
{
	bool result;

	result = m_direct3D->Render();
	if (!result)
	{
		return false;
	}
	
	return true;
}
