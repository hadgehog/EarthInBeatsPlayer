#pragma once

#include <string>
#include <wrl.h>
#include <d3d12.h>

struct TextureHandle 
{
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu {};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    bool valid() const 
    { 
        return m_resource != nullptr;
    }
};

struct Material 
{
    std::string m_materialName;
    std::string m_baseColorPath;        // asset path
    TextureHandle m_baseColorTexture;   // resolved handle
};