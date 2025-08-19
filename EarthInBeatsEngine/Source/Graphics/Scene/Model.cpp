#include "Model.h"
#include "../Helpers/DxHelper.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static void ExtractMesh(const aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    vertices.resize(mesh->mNumVertices);

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        auto& v = vertices[i];

        v.pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        if (mesh->HasNormals())
            v.norm = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        else
            v.norm = { 0,1,0 };

        if (mesh->HasTextureCoords(0))
            v.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        else
            v.uv = { 0,0 };
    }

    indices.clear();

    for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];

        if (face.mNumIndices == 3)
        {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }
}

bool Model::Initialize(ID3D12Device* device, const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_GenNormals |
        aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices |
        aiProcess_ConvertToLeftHanded);

    if (!scene || !scene->mRootNode)
    {
        return false;
    }

    // Materials
    m_materials.clear();
    m_materials.resize(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        Material mat {};
        mat.m_materialName = scene->mMaterials[i]->GetName().C_Str();

        aiString texPath;
        if (scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS)
            mat.m_baseColorPath = texPath.C_Str();

        m_materials[i] = std::move(mat);
    }

    // Mesh data
    m_vertices.clear();
    m_indices.clear();
    m_meshMaterialIndices.clear();

    m_vertices.reserve(scene->mNumMeshes);
    m_indices.reserve(scene->mNumMeshes);
    m_meshMaterialIndices.reserve(scene->mNumMeshes);

    for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
            continue;

        std::vector<Vertex> verts;
        std::vector<uint32_t> idxs;

        ExtractMesh(mesh, verts, idxs);

        m_vertices.push_back(std::move(verts));
        m_indices.push_back(std::move(idxs));
        m_meshMaterialIndices.push_back(mesh->mMaterialIndex);
    }

    m_position = { 0,0,0 }; 
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    m_scale = 1.0f;

    this->CreateResources(device);

    return true;
}

void Model::CreateResources(ID3D12Device* device)
{
    m_meshes.resize(m_vertices.size());

    for (size_t i = 0; i < m_vertices.size(); ++i)
    {
        m_meshes[i].Initialize(device, m_vertices[i], m_indices[i], m_meshMaterialIndices[i]);
    }
}

void Model::ResolveMaterials(TextureManager& texMgr, const UploadContext& up)
{
    for (auto& material : m_materials)
    {
        if (!material.m_baseColorPath.empty())
        {
            material.m_baseColorTexture = texMgr.LoadTexture(up, material.m_baseColorPath);
        }
    }
}

void Model::OverrideAllBaseColor(TextureManager& texMgr, const UploadContext& up, const std::string& path)
{
    if (path.empty()) 
        return;

    for (auto& material : m_materials)
    {
        material.m_baseColorTexture = texMgr.LoadTexture(up, path);
    }
}

const std::vector<Mesh>& Model::Meshes() const
{
	return m_meshes;
}

const Material& Model::MaterialAt(uint32_t idx) const
{
	return m_materials.at(idx);
}

void Model::Translate(float dX, float dY, float dZ)
{
    m_position.x += dX; 
    m_position.y += dY;
    m_position.z += dZ;
}

void Model::Rotate(float yaw, float pitch)
{
    m_yaw += yaw; 
    m_pitch += pitch;
}

void Model::Scale(float factor)
{
    this->SetScale(m_scale * factor);
}

float Model::GetScale() const
{
	return m_scale;
}

void Model::SetScale(float scale)
{
    if (scale < 0.01f)
        scale = 0.01f;

    if (scale > 100.0f)
        scale = 100.0f;

    m_scale = scale;
}

DirectX::XMMATRIX Model::ModelMatrix() const
{
    return
        DirectX::XMMatrixScaling(m_scale, m_scale, m_scale) *
        DirectX::XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f) *
        DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
}
