#pragma once

#include <d3d12.h>
#include <cstdint>

struct UploadContext 
{
    ID3D12Device* m_device = nullptr;
    ID3D12CommandQueue* m_queue = nullptr;
    ID3D12CommandAllocator* m_alloc = nullptr;
    ID3D12GraphicsCommandList* m_list = nullptr;
    ID3D12Fence* m_fence = nullptr;
    HANDLE m_fenceEvent = 0;
    uint64_t* m_fenceValue = nullptr;
};
