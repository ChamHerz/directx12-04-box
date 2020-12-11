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