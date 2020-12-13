//***************************************************************************************
// Resource.h by ChamHerz (C) 2020
//***************************************************************************************

#pragma once

#include <d3d12.h>

#include <wrl.h>
using namespace Microsoft::WRL;

class Resource
{
public:
	static void resourceBarrier(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES fromState,
		D3D12_RESOURCE_STATES toState
	);

	static ComPtr<ID3D12Resource> buffer(
		ID3D12Device* device,
		SIZE_T sizeInBytes,
		D3D12_HEAP_TYPE heapType
	);
};

