#include "Model.h"
#include "../Helpers/DxHelper.h"

bool Model::Initialize(const std::string& path, std::string* outPrimaryTexPath)
{
	return false;
}

void Model::CreateResources(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
}

void Model::Draw(ID3D12GraphicsCommandList* cmdList) const
{
}

void Model::Translate(float dX, float dY, float dZ)
{
}

void Model::Rotate(float yaw, float pitch)
{
}

void Model::Scale(float factor)
{
}

float Model::GetScale() const
{
	return 0.0f;
}

void Model::SetScale(float scale)
{
}

DirectX::XMMATRIX Model::ModelMatrix() const
{
	return DirectX::XMMATRIX();
}

const std::string& Model::GetTexturePath() const
{
	return std::string();
}

void Model::SetTexturePath(const std::string& path)
{
}
