#include "Mesh.h"
#include "../Helpers/d3dx12.h"
#include "../Helpers/DxHelper.h"

void Mesh::Initialize(
	ID3D12Device* device, 
	const std::vector<Vertex>& vertices, 
	const std::vector<uint32_t>& indices, 
	UINT materialIndex)
{
    m_materialIndex = materialIndex;
    m_indexCount = (UINT)indices.size();

    size_t vbSize = vertices.size() * sizeof(Vertex);
    size_t ibSize = indices.size() * sizeof(uint32_t);

    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
    auto vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);
    auto ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);

    DX::ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));
    DX::ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

    void* p = nullptr;

    if (vbSize) 
    { 
        DX::ThrowIfFailed(m_vertexBuffer->Map(0, nullptr, &p)); std::memcpy(p, vertices.data(), vbSize); m_vertexBuffer->Unmap(0, nullptr);
    }

    if (ibSize) 
    { 
        DX::ThrowIfFailed(m_indexBuffer->Map(0, nullptr, &p)); std::memcpy(p, indices.data(), ibSize); m_indexBuffer->Unmap(0, nullptr);
    }

    m_vertexBufferView = { m_vertexBuffer->GetGPUVirtualAddress(), (UINT)sizeof(Vertex), (UINT)vbSize };
    m_indexBufferView = { m_indexBuffer->GetGPUVirtualAddress(), (UINT)ibSize, DXGI_FORMAT_R32_UINT };
}

void Mesh::Draw(ID3D12GraphicsCommandList* cmdList) const
{
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    cmdList->IASetIndexBuffer(&m_indexBufferView);
    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}

UINT Mesh::MaterialIndex() const
{
	return m_materialIndex;
}
