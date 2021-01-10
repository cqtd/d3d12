#include "SystemClass.h"

SystemClass::SystemClass()
{
	m_input = 0;
	m_graphics = 0;
}

SystemClass::SystemClass(const SystemClass&)
{
}

SystemClass::~SystemClass()
{
}

bool SystemClass::Initialize()
{
	int screenHeight, screenWidth;
	bool result;

	screenHeight = 0;
	screenWidth = 0;

	InitializeWindows(screenHeight, screenWidth);

	m_input = new InputClass;
	if (!m_input)
	{
		return false;
	}

	m_input->Initialize();

	m_graphics = new GraphicsClass;
	if (!m_graphics)
	{
		return false;
	}

	result = m_graphics->Initialize(screenHeight, screenWidth, m_hwnd);
	if (!result)
	{
		return false;
	}

	return true;
}

void SystemClass::Shutdown()
{
	if (m_graphics)
	{
		m_graphics->Shutdown();
		delete m_graphics;

		m_graphics = 0;
	}

	if (m_input)
	{
		delete m_input;
		m_input = 0;
	}

	ShutdownWindows();
}

void SystemClass::Run()
{
	MSG msg;
	bool done, result;

	ZeroMemory(&msg, sizeof(MSG));

	done = false;

	while(!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			result = Frame();
			if (!result)
			{
				done = true;
			}
		}
	}
}

LRESULT SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch(umsg)
	{
	case WM_KEYDOWN:
		{
		m_input->KeyDown((unsigned int)wparam);
		return 0;
		}
	case WM_KEYUP:
		{
		m_input->KeyUp((unsigned int)wparam);
		return 0;
		}

	default:
		{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
		}
	}
}

bool SystemClass::Frame()
{
	bool result;

	if (m_input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	result = m_graphics->Frame();
	if (!result)
	{
		return false;
	}

	return true;
}

void SystemClass::InitializeWindows(int& screenHeight, int& screenWidth)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	ApplicationHandle = this;

	m_hInstance = GetModuleHandle(NULL);

	m_applicationName = L"DX12 Engine";

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	RegisterClassEx(&wc);

	screenHeight = GetSystemMetrics(SM_CYSCREEN);
	screenWidth = GetSystemMetrics(SM_CXSCREEN);

	if (FULL_SCREEN)
	{
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		posX = posY = 0;
	}else
	{
		screenWidth = 1600;
		screenHeight = 900;

		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth / 2);
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight / 2);
	}

	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW, m_applicationName, m_applicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, screenWidth, screenHeight, NULL, NULL, m_hInstance, NULL);

	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	ShowCursor(false);

	return;
}

void SystemClass::ShutdownWindows()
{
	ShowCursor(true);

	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	UnregisterClass(m_applicationName, m_hInstance);
	ApplicationHandle = NULL;
}

LRESULT __stdcall WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch(umsg)
	{
	case WM_DESTROY:
		{
		PostQuitMessage(0);
		return 0;
		}
	case WM_CLOSE:
		{
		PostQuitMessage(0);
		return 0;
		}

	default:
		{
		return ApplicationHandle->MessageHandler(hwnd, umsg, wparam, lparam);
		}
	}
}
