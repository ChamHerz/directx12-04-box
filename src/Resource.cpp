#include "Resource.h"

// Cambiar el estado de un recurso
void Resource::resourceBarrier(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Resource* resource,
	D3D12_RESOURCE_STATES fromState,
	D3D12_RESOURCE_STATES toState
)
{
	D3D12_RESOURCE_BARRIER resourceBarrierDesc{};
	resourceBarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrierDesc.Transition.pResource = resource;
	resourceBarrierDesc.Transition.StateBefore = fromState;
	resourceBarrierDesc.Transition.StateAfter = toState;
	resourceBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(
		1,
		&resourceBarrierDesc
	);
}

ComPtr<ID3D12Resource> Resource::buffer(
	ID3D12Device* device,
	SIZE_T sizeInBytes,
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD
)
{
	ComPtr<ID3D12Resource> resource;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;

	D3D12_HEAP_PROPERTIES HeapProps{};
	HeapProps.Type = heapType;

	device->CreateCommittedResource(
		&HeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	return resource;
}