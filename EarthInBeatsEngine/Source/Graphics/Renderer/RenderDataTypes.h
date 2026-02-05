#pragma once

#include <d3d11_4.h>
#include <DirectXMath.h>
#include <vector>

namespace DX
{
	struct Vertex
	{
		DirectX::XMFLOAT3 Position = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT2 TexCoord = { 0.0f, 0.0f };
		DirectX::XMFLOAT3 Normal = { 0.0f,0.0f,0.0f };
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;
	};
}