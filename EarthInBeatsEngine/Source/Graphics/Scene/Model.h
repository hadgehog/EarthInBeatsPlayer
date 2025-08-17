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

class Model
{
public:
    Model() = default;
    ~Model() = default;

    // Load model from file
    bool Initialize(const std::string& path, std::string* outPrimaryTexPath = nullptr);

    // GPU resources
    void CreateResources(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    void Draw(ID3D12GraphicsCommandList* cmdList) const;

    // Transform controls
    void Translate(float dX, float dY, float dZ);
    void Rotate(float yaw, float pitch);
    void Scale(float factor);

    float GetScale() const;
    void SetScale(float scale);

    DirectX::XMMATRIX ModelMatrix() const;

protected:
    // Optional explicit texture path override
    const std::string& GetTexturePath() const;
    void SetTexturePath(const std::string& path);

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::string m_texturePath; // may come from material or user override

    // CPU-side transform
    DirectX::XMFLOAT3 m_position { 0,0,0 };
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_scale = 1.0f;

    // GPU buffers
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView {};
    D3D12_INDEX_BUFFER_VIEW  m_indexBufferView {};
};
