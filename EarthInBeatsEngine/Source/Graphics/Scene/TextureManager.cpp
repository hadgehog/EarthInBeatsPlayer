#include "TextureManager.h"
#include "../Helpers/d3dx12.h"
#include "../Helpers/DxHelper.h"

#include <DirectXTex.h>

void TextureManager::Initialize(ID3D12Device* device, ID3D12DescriptorHeap* shaderVisibleSrvHeap, UINT descriptorSize)
{
	m_device = device;
	m_srvHeap = shaderVisibleSrvHeap;
	m_descriptorSize = descriptorSize;
	m_nextSlot = 0;
	//m_cache.clear();
}

TextureHandle TextureManager::LoadTexture(const UploadContext& ctx, const std::vector<uint8_t>& textureData)
{
	//auto texIt = m_cache.find(textureData);

	//if (texIt != m_cache.end())
	//	return texIt->second;

	TextureHandle handle = this->LoadInternal(ctx, textureData);

	//if (handle.valid()) 
	//	m_cache[textureData] = handle;

	return handle;
}

TextureHandle TextureManager::LoadInternal(const UploadContext& ctx, const std::vector<uint8_t>& textureData)
{
    TextureHandle handle {};
    DirectX::ScratchImage img;

    HRESULT hr = LoadFromDDSMemory(textureData.data(), textureData.size(), DirectX::DDS_FLAGS::DDS_FLAGS_FORCE_RGB, nullptr, img);

    if (FAILED(hr))
    {
        hr = LoadFromTGAMemory(textureData.data(), textureData.size(), DirectX::TGA_FLAGS::TGA_FLAGS_NONE, nullptr, img);
    }

    if (FAILED(hr))
    {
        hr = LoadFromWICMemory(textureData.data(), textureData.size(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_RGB, nullptr, img);
    }

    if (FAILED(hr))
    {
        return handle;
    }

    const DirectX::Image* simg = img.GetImage(0, 0, 0);

    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(simg->format, simg->width, simg->height, 1, 1);
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);

    Microsoft::WRL::ComPtr<ID3D12Resource> tex;
    DX::ThrowIfFailed(ctx.m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&tex)));

    const UINT64 uploadSize = GetRequiredIntermediateSize(tex.Get(), 0, 1);
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

    Microsoft::WRL::ComPtr<ID3D12Resource> upload;
    DX::ThrowIfFailed(ctx.m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)));

    D3D12_SUBRESOURCE_DATA sub{}; 
    sub.pData = simg->pixels; 
    sub.RowPitch = simg->rowPitch; 
    sub.SlicePitch = simg->slicePitch;

    DX::ThrowIfFailed(ctx.m_alloc->Reset());
    DX::ThrowIfFailed(ctx.m_list->Reset(ctx.m_alloc, nullptr));

    UpdateSubresources(ctx.m_list, tex.Get(), upload.Get(), 0, 0, 1, &sub);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(tex.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    ctx.m_list->ResourceBarrier(1, &barrier);

    DX::ThrowIfFailed(ctx.m_list->Close());

    ID3D12CommandList* lists[] = { ctx.m_list };
    ctx.m_queue->ExecuteCommandLists(1, lists);

    const uint64_t fenceToWait = ++(*ctx.m_fenceValue);

    DX::ThrowIfFailed(ctx.m_queue->Signal(ctx.m_fence, fenceToWait));

    if (ctx.m_fence->GetCompletedValue() < fenceToWait) 
    {
        DX::ThrowIfFailed(ctx.m_fence->SetEventOnCompletion(fenceToWait, ctx.m_fenceEvent));
        WaitForSingleObject(ctx.m_fenceEvent, INFINITE);
    }

    UINT slot = m_nextSlot++;
    handle.m_cpu = this->CpuAt(slot);
    handle.m_gpu = this->GpuAt(slot);
    handle.m_resource = tex;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv {};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = simg->format;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(tex.Get(), &srv, handle.m_cpu);

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CpuAt(UINT slot) const
{
    auto base = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE handle = { base.ptr + SIZE_T(slot) * SIZE_T(m_descriptorSize) };
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GpuAt(UINT slot) const
{
    auto base = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE handle = { base.ptr + UINT64(slot) * UINT64(m_descriptorSize) };
    return handle;
}
