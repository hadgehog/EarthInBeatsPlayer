#pragma once

#include <string>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>

#include "../Renderer/UploadContext.h"

#include "Material.h"

class TextureManager
{
public:
    TextureManager() = default;
    ~TextureManager() = default;

    void Initialize(ID3D12Device* device, ID3D12DescriptorHeap* shaderVisibleSrvHeap, UINT descriptorSize);
    TextureHandle LoadTexture(const UploadContext& ctx, const std::vector<uint8_t>& textureData);

private:
    TextureHandle LoadInternal(const UploadContext& up, const std::vector<uint8_t>& textureData);
    D3D12_CPU_DESCRIPTOR_HANDLE CpuAt(UINT slot) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuAt(UINT slot) const;

    ID3D12Device* m_device = nullptr;
    ID3D12DescriptorHeap* m_srvHeap = nullptr; // shader-visible
    UINT m_descriptorSize = 0;
    UINT m_nextSlot = 0;
    //std::unordered_map<std::vector<uint8_t>, TextureHandle> m_cache;  // TODO improve (vector as a key is bad)
};

