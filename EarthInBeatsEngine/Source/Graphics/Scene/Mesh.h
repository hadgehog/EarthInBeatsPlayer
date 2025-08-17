#pragma once

#include <vector>
#include <string>
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

struct Vertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 norm;
    DirectX::XMFLOAT2 uv;
};

class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    void Initialize(ID3D12Device* device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        UINT materialIndex);

    void Draw(ID3D12GraphicsCommandList* cmdList) const;

    UINT MaterialIndex() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView {};
    D3D12_INDEX_BUFFER_VIEW  m_indexBufferView {};

    UINT m_indexCount = 0;
    UINT m_materialIndex = 0;
};
