//***************************************************************************************
// Resource.h by ChamHerz (C) 2020
//***************************************************************************************

#pragma once

#include <d3d12.h>

class Resource
{
public:
	static void resourceBarrier(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES fromState,
		D3D12_RESOURCE_STATES toState
	);
};

