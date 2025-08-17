#include "TextureManager.h"
#include "../Helpers/d3dx12.h"

#include <DirectXTex.h>

void TextureManager::Initialize(ID3D12Device* device, ID3D12DescriptorHeap* shaderVisibleSrvHeap, UINT descriptorSize)
{
}

TextureHandle TextureManager::LoadTexture(const UploadContext& up, const std::string& path)
{
	return TextureHandle();
}

TextureHandle TextureManager::LoadBackgroundTexture(const UploadContext& up, const std::string& path)
{
	return TextureHandle();
}

TextureHandle TextureManager::LoadInternal(const UploadContext& up, const std::string& path)
{
	return TextureHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CpuAt(UINT slot) const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GpuAt(UINT slot) const
{
	return D3D12_GPU_DESCRIPTOR_HANDLE();
}
