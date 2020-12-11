//***************************************************************************************
// Engine.h by ChamHerz (C) 2020
//***************************************************************************************

#pragma once

// Activa los features para adiministrar la memoria en Debug
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <windows.h>

// para wstring
#include <string>
using namespace std;

//DirectX12
#include <d3d12.h>				// para el Device
#include <dxgi1_6.h>			// para el IDXGIFactory4

// Punteros Inteligentes
#include <wrl.h>
using namespace Microsoft::WRL;

// Propio del Engine
#include "Resource.h"
#include "TimeController.h"

static const UINT8 chainSize = 2;

class Engine
{
protected:
	// constructor protegido SINGLETON
	Engine(HINSTANCE hInstance);
	Engine(const Engine& rhs) = delete;
	Engine& operator=(const Engine& rhs) = delete;
	virtual ~Engine();

	static Engine* engine;
	HINSTANCE engineInstance = nullptr;			// instancia de la aplicacion
	HWND      engineHwnd = nullptr;				// handle de la ventana principal

	bool initMainWindow();
	bool initDirectX();
	int clientWidth = 800;
	int clientHeight = 600;
	wstring engineCaption = L"Shader Game";

	// ==== DIRECTX12 ==== //

	// Dispositivos
	ComPtr<IDXGIFactory4> factory;				// Constructor de Interfaces
	ComPtr<ID3D12Device> device;				// Dispositivo (virtual)

	void createDevice();

	// Colas
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	void createQueues();

	// Fences
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	UINT64 fenceValue;
	void createFence();
	void flushAndWait();

	// Swapchain
	ComPtr<IDXGISwapChain3> swapchain;
	ComPtr<ID3D12DescriptorHeap> renderTargetViewHeap;
	ComPtr<ID3D12Resource> renderTargets[chainSize];
	void createSwapchain(HWND hWnd, UINT Width, UINT Height);
	void createRenderTargets();

	// Command List para renderizar
	void recordCommandList();

	void update();

	// Control de Frames
	TimeController timeController;
	void calculateFrameStats();

public:
	static Engine* getEngine();

	virtual bool initialize();

	int run();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};