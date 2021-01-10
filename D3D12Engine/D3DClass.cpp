#include "D3DClass.h"

D3DClass::D3DClass()
{
	m_device = 0;
	m_commandQueue = 0;
	m_swapChain = 0;
	m_renderTargetViewHeap = 0;
	m_backBufferRenderTarget[0] = 0;
	m_backBufferRenderTarget[1] = 0;
	m_commandAllocator = 0;
	m_commandList = 0;
	m_pipelineState = 0;
	m_fence = 0;
	m_fenceEvent = 0;
}

D3DClass::D3DClass(const D3DClass&)
{
}

D3DClass::~D3DClass()
{
}

bool D3DClass::Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
{
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT result;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	IDXGIFactory4* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator, renderTargetViewDescriptorSize;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	IDXGISwapChain* swapChain;

	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;

	// store vsync setting
	m_vsync_enabled = vsync;

	featureLevel = D3D_FEATURE_LEVEL_12_1;

	result = D3D12CreateDevice(NULL, featureLevel, __uuidof(ID3D12Device), (void**)&m_device);
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not create a DirectX 12.1 device. The default video card does not support DirectX 12.1.",
			L"DirectX Device Failure", MB_OK);
		return false;
	}

	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	result = m_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_commandQueue);
	if (FAILED(result))
	{
		return false;
	}

	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED,
		&numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	for (i = 0; i< numModes; i++)
	{
		if (displayModeList[i].Height == (unsigned int) screenHeight)
		{
			if (displayModeList[i].Width == (unsigned int) screenWidth)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	delete[] displayModeList;
	displayModeList = 0;

	adapterOutput->Release();
	adapterOutput = 0;

	adapter->Release();
	adapter = 0;

	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	swapChainDesc.BufferCount = 2;

	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Width = screenWidth;

	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	swapChainDesc.OutputWindow = hwnd;

	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// turn multisampling off
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swapChainDesc.Flags = 0;

	// create the swap chain
	result = factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		return false;
	}

	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)(&m_swapChain));
	if (FAILED(result))
	{
		return false;
	}

	swapChain = 0;

	factory->Release();
	factory = 0;

	// render targets and back buffers
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));

	renderTargetViewHeapDesc.NumDescriptors = 2;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if (FAILED(result))
	{
		return false;
	}

	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	result = m_swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if (FAILED(result) )
	{
		return false;
	}

	m_device->CreateRenderTargetView(m_backBufferRenderTarget[0], NULL, renderTargetViewHandle);

	// increment the view handle to the next descriptor location in the render target view heap
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	result = m_swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if (FAILED(result))
	{
		return false;
	}

	m_device->CreateRenderTargetView(m_backBufferRenderTarget[1], NULL, renderTargetViewHandle);

	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();


	// command allocator
	result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator),
		(void**)&m_commandAllocator);
	if (FAILED(result) )
	{
		return false;
	}

	// basic command list
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, NULL,
		__uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if (FAILED(result))
	{
		return false;
	}

	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	// fence gpu sync
	result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	if (FAILED(result))
	{
		return false;
	}

	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL)
	{
		return false;
	}

	m_fenceValue = 1;

	return true;
}

void D3DClass::Shutdown()
{
	int error;

	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	error = CloseHandle(m_fenceEvent);
	if (error == 0)
	{
		
	}

	if (m_fence)
	{
		m_fence->Release();
		m_fence = 0;
	}

	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = 0;
	}

	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = 0;
	}

	if (m_backBufferRenderTarget[0])
	{
		m_backBufferRenderTarget[0]->Release();
		m_backBufferRenderTarget[0] = 0;
	}
	if (m_backBufferRenderTarget[1])
	{
		m_backBufferRenderTarget[1]->Release();
		m_backBufferRenderTarget[1] = 0;
	}

	if (m_renderTargetViewHeap)
	{
		m_renderTargetViewHeap->Release();
		m_renderTargetViewHeap = 0;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = 0;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}
}

bool D3DClass::Render()
{
	HRESULT result;

	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];
	unsigned long long fenceToWaitFor;

	result = m_commandAllocator->Reset();
	if (FAILED(result))
	{
		return false;
	}

	result = m_commandList->Reset(m_commandAllocator, m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}

	// resource barrier to synchronize the next back buffer for render

	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBufferRenderTarget[m_bufferIndex];
	
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	m_commandList->ResourceBarrier(1, &barrier);

	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	if (m_bufferIndex ==1)
	{
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, NULL);

	// color

	color[0] = 0.5;
	color[1] = 0.5;
	color[2] = 0.5;
	color[3] = 1.0;


	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);

	// indicate that back buffer will now be used to present

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	m_commandList->ResourceBarrier(1, &barrier);

	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	ppCommandLists[0] = m_commandList;

	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

	if (m_vsync_enabled)
	{
		result = m_swapChain->Present(1, 0);
		if (FAILED(result))
		{
			return false;
		}
	}
	else
	{
		result = m_swapChain->Present(0, 0);
		if (FAILED(result))
		{
			return false;
		}
	}

	// setup fence to synchronize and let us know when the gpu is complete rendering.

	fenceToWaitFor = m_fenceValue;
	result = m_commandQueue->Signal(m_fence, fenceToWaitFor);

	if (FAILED(result))
	{
		return false;
	}

	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
		if (FAILED(result))
		{
			return false;
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	//for the next frame swap to the other back buffer using the alternating index.

	m_bufferIndex == 0 ? m_bufferIndex = 1 : m_bufferIndex = 0;

	return true;
}
