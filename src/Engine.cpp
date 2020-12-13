#include <cassert>
#include "Engine.h"

// Para los colores
#include <DirectXColors.h>
using namespace DirectX;

Engine* Engine::engine = nullptr;
Engine* Engine::getEngine()
{
	return engine;
}

Engine::Engine(HINSTANCE hInstance) : engineInstance(hInstance)
{
	assert(engine == nullptr);
	engine = this;
}

Engine::~Engine()
{
	if (device != nullptr)
		flushAndWait();
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Reenviamos el hwnd, xq despues del CreateWindows el hwnd es valido
	return Engine::getEngine()->MsgProc(hwnd, msg, wParam, lParam);
}

LRESULT Engine::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_DESTROY se manda cuando comienza a destruirse
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Engine::initMainWindow()
{
	// inciar la ventana principal
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = engineInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"El registro de la clase fallo", 0, 0);
		return false;
	}

	// Creamos un rectangulo en base a las dimensiones del cliente
	RECT R = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	engineHwnd = CreateWindow(L"MainWnd", engineCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, engineInstance, 0);
	if (!engineHwnd)
	{
		MessageBox(0, L"Error al crear la ventana principal.", 0, 0);
		return false;
	}

	ShowWindow(engineHwnd, SW_SHOW);
	UpdateWindow(engineHwnd);

	return true;
}

bool Engine::initDirectX()
{
	// iniciamos el DirectX 12
	createDevice();
	createQueues();
	createFence();

	createSwapchain(engineHwnd, clientWidth, clientHeight);
	createRenderTargets();

	createRootSignature();
	createPipeline();

	return true;
}

bool Engine::initialize()
{
	if (!initMainWindow())
		return false;

	if (!initDirectX())
		return false;

	return true;
}

int Engine::run()
{
	MSG msg = { 0 };

	timeController.reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timeController.update();
			calculateFrameStats();
			update();
		}
	}

	return (int)msg.wParam;
}

void Engine::calculateFrameStats()
{
	static int frameCounter = 0;
	static float timeElapsed = 0.0f;

	frameCounter++;

	if ((timeController.totalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCounter;
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = engineCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(engineHwnd, windowText.c_str());

		// Reseteo para el sieguiente promedio
		frameCounter = 0;
		timeElapsed += 1.0f;
	}
}

void Engine::update()
{
	recordCommandList();
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, ppCommandLists);
	swapchain->Present(1, 0);
	flushAndWait();
}

void Engine::createDevice()
{
	// Creamos el Factory
	CreateDXGIFactory1(IID_PPV_ARGS(&factory));

	// es un puntero inteligente, se destruye al salir de la funcion
	ComPtr<IDXGIAdapter1> adapter;

	// buscamos un adaptador e intentamos crear el device
	bool adapterFound = false;
	for (UINT adapterIndex = 0;
		!adapterFound && factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
		++adapterIndex)
	{
		// Extraemos los datos del adapter actual
		DXGI_ADAPTER_DESC1 AdapterDesc;
		adapter->GetDesc1(&AdapterDesc);

		// Una placa de video tiene este flag
		if (AdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		// al usar el nullptr, solo revisa si el adaptador se banca el DirectX
		const HRESULT result = D3D12CreateDevice(
			adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(result))
		{
			adapterFound = true;
		}
	}

	// Ahora si creamos el Device y lo guardamos en el puntero inteligente
	D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
}

void Engine::createQueues()
{
	// preparamos y creamos la cola de comandos
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc{};
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	CommandQueueDesc.NodeMask = 0;
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&commandQueue));

	// creamos el allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	// creamos y cerramos la lista de comandos
	device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(),
		nullptr, // estado inicial del pipeline
		IID_PPV_ARGS(&commandList)
	);
	commandList->Close();
}

void Engine::createFence()
{
	fenceValue = 0;
	device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Engine::flushAndWait()
{
	const UINT64 FenceValueToSignal = fenceValue;
	commandQueue->Signal(fence.Get(), FenceValueToSignal);

	++fenceValue;

	if (fence->GetCompletedValue() < FenceValueToSignal)
	{
		fence->SetEventOnCompletion(FenceValueToSignal, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void Engine::createSwapchain(HWND hWnd, UINT Width, UINT Height)
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
	swapchainDesc.BufferCount = 2;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.Width = Width;
	swapchainDesc.Height = Height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.SampleDesc.Count = 1;

	// Creamos la SwapChain
	ComPtr<IDXGISwapChain1> swapchainTemp;
	factory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hWnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		&swapchainTemp
	);

	swapchainTemp.As(&swapchain);
}

void Engine::createRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = chainSize;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&renderTargetViewHeap));

	for (UINT FrameIndex = 0; FrameIndex < chainSize; ++FrameIndex)
	{
		swapchain->GetBuffer(FrameIndex, IID_PPV_ARGS(&renderTargets[FrameIndex]));

		D3D12_RENDER_TARGET_VIEW_DESC RTDesc{};
		RTDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		RTDesc.Texture2D.MipSlice = 0;
		RTDesc.Texture2D.PlaneSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		DestDescriptor.ptr += ((SIZE_T)FrameIndex) * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		device->CreateRenderTargetView(renderTargets[FrameIndex].Get(), &RTDesc, DestDescriptor);
	}
}

void Engine::createRootSignature()
{
	/* RootSignature vacío, no le pasamos decriptores
	  (ni texturas, ni constant buffer, etc.,) a los shaders */

	D3D12_ROOT_SIGNATURE_DESC signatureDesc{};
	signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	signatureDesc.NumParameters = 0;
	signatureDesc.NumStaticSamplers = 0;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void Engine::createPipeline()
{
	/* Shaders */

	ComPtr<ID3DBlob> vertexBlob = loadShader(L"shader\\shaders.hlsl", "VSMain", "vs_5_1");

	D3D12_SHADER_BYTECODE vertexShaderBytecode;
	vertexShaderBytecode.pShaderBytecode = vertexBlob->GetBufferPointer();
	vertexShaderBytecode.BytecodeLength = vertexBlob->GetBufferSize();

	ComPtr<ID3DBlob> pixelBlob = loadShader(L"shader\\shaders.hlsl", "PSMain", "ps_5_1");

	D3D12_SHADER_BYTECODE pixelShaderBytecode;
	pixelShaderBytecode.pShaderBytecode = pixelBlob->GetBufferPointer();
	pixelShaderBytecode.BytecodeLength = pixelBlob->GetBufferSize();

	/* Input Layout */
	  // vamos a usar un Vertex con position y color
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		// SemanticName; SemanticIndex; Format; InputSlot; AlignedByteOffset; InputSlotClass; InstanceDataStepRate;
		  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 3 * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC inputLayout{};
	inputLayout.NumElements = 2;
	inputLayout.pInputElementDescs = inputElementDescs;

	/* Rasterizer Stage */

	D3D12_RASTERIZER_DESC rasterizerState{};
	rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerState.FrontCounterClockwise = FALSE;
	rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerState.DepthClipEnable = TRUE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	rasterizerState.ForcedSampleCount = 0;
	rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	/* Blend State */

	D3D12_BLEND_DESC blendState{};
	blendState.AlphaToCoverageEnable = FALSE;
	blendState.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
	  FALSE,FALSE,
	  D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
	  D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
	  D3D12_LOGIC_OP_NOOP,
	  D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		blendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}

	/* Depth Stencil */

	D3D12_DEPTH_STENCIL_DESC depthStencilState{};
	depthStencilState.DepthEnable = FALSE;
	depthStencilState.StencilEnable = FALSE;

	/* Pipeline */

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = inputLayout;
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.RasterizerState = rasterizerState;
	psoDesc.BlendState = blendState;
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
}

ComPtr<ID3DBlob> Engine::loadShader(LPCWSTR filename, LPCSTR entryPoint, LPCSTR target)
{
	HRESULT hr;

	/* Shaders */

	ComPtr<ID3DBlob> shaderBlob;
	hr = D3DCompileFromFile(
		filename, // FileName
		nullptr, nullptr, // MacroDefines, Includes, 		
		entryPoint, // FunctionEntryPoint
		target, // Target: "vs_5_0", "ps_5_0", "vs_5_1", "ps_5_1"
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // Compile flags
		0, // Flags2
		&shaderBlob, // Code
		nullptr // Error
	);
	if (FAILED(hr))
	{
		OutputDebugStringA("[ERROR] D3DCompileFromFile -- Vertex shader");
	}

	return shaderBlob;
}

void Engine::recordCommandList()
{
	const UINT backFrameIndex = swapchain->GetCurrentBackBufferIndex();

	// para borrar la pantalla no necesitamos un pipeline
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), pipelineState.Get());

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptor = renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetDescriptor.ptr += ((SIZE_T)backFrameIndex) * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	Resource::resourceBarrier(commandList.Get(), renderTargets[backFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// comando para pintar la pantalla
	commandList->ClearRenderTargetView(renderTargetDescriptor, Colors::LightSteelBlue, 0, nullptr);

	Resource::resourceBarrier(commandList.Get(), renderTargets[backFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	commandList->Close();
}