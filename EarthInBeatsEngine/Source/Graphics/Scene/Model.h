#pragma once

#include "Mesh.h"
#include "Material.h"
#include "TextureManager.h"

class Model
{
public:
    Model() = default;
    ~Model() = default;

    // Load model from file
    bool Initialize(ID3D12Device* device, const std::vector<uint8_t>& modelData);

    void ResolveMaterials(TextureManager& texMgr, const UploadContext& ctx);
    void OverrideAllBaseColor(TextureManager& texMgr, const UploadContext& ctx, const std::vector<uint8_t>& data);

    const std::vector<Mesh>& Meshes() const;
    const Material& MaterialAt(uint32_t idx) const;

    // Transform controls
    void Translate(float dX, float dY, float dZ);
    void Rotate(float yaw, float pitch);
    void Scale(float factor);

    float GetScale() const;
    void SetScale(float scale);

    DirectX::XMMATRIX ModelMatrix() const;

protected:
    void CreateResources(ID3D12Device* device);

private:
    std::vector<std::vector<Vertex>> m_vertices;
    std::vector<std::vector<uint32_t>> m_indices;
    std::vector<uint32_t> m_meshMaterialIndices;

    std::vector<Mesh> m_meshes;
    std::vector<Material> m_materials;

    DirectX::XMFLOAT3 m_position { 0,0,0 };
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_scale = 1.0f;
};
