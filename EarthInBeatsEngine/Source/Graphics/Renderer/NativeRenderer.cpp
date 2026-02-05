#include "NativeRenderer.h"
#include "../Helpers/DxHelper.h"

#include <d3dcompiler.h>    // D3DCompile for HLSL
#include <DirectXTex.h>     // CreateTexture

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void NativeRenderer::Initialize(
	const winrt::com_ptr<ISwapChainPanelNative>& panel,
    const uint32_t width,
    const uint32_t height,
	const std::vector<uint8_t>& model, 
	const std::vector<uint8_t>& bgTex)
{
    m_nativePanel = panel;

    m_width = width;
    m_height = height;

    m_pendingResize = true;

	this->InitD3D11();

	this->CreateBackgroundPipeline();
	this->CreateModelPipeline();

	this->LoadBackgroundTexture(bgTex);
	this->LoadModelAssimp(model);

	this->RenderFrame();
}

void NativeRenderer::RenderFrame()
{
    DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationY(0), DirectX::XMMatrixTranslation(0, 0, 10));

    modelMatrix = DirectX::XMMatrixMultiply(
        modelMatrix,
        DirectX::XMMatrixScaling(m_modelScale, m_modelScale, m_modelScale));

    // update CB
    DirectX::XMStoreFloat4x4(&this->m_modelCBData.model, DirectX::XMMatrixTranspose(modelMatrix));

    // set viewport
    D3D11_VIEWPORT vp { 0 };
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_d3d11Context->RSSetViewports(1, &vp);

    // bind RTV
    float clear[4] = { 0.05f, 0.05f, 0.08f, 1.0f };
    m_d3d11Context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
    m_d3d11Context->ClearRenderTargetView(m_renderTargetView.Get(), clear);

    // ---------- background ----------
    if (m_bgShaderResourceView && m_bgVertexShader && m_bgPixelShader)
    {
        // no input layout / no VB
        m_d3d11Context->IASetInputLayout(nullptr);
        m_d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_d3d11Context->VSSetShader(m_bgVertexShader.Get(), nullptr, 0);
        m_d3d11Context->PSSetShader(m_bgPixelShader.Get(), nullptr, 0);

        m_d3d11Context->PSSetShaderResources(0, 1, m_bgShaderResourceView.GetAddressOf());
        m_d3d11Context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        m_d3d11Context->Draw(3, 0);

        // avoid D3D11 warning about bound SRV when later used as RTV (not necessary)
        ID3D11ShaderResourceView* nullSrv[1] = { nullptr };
        m_d3d11Context->PSSetShaderResources(0, 1, nullSrv);
    }

    // ---------- model ----------
    if (m_modelVertexBuffer && m_modelIndexBuffer && m_indexCount > 0)
    {
        UINT stride = sizeof(DX::Vertex);
        UINT offset = 0;

        m_d3d11Context->IASetVertexBuffers(0, 1, m_modelVertexBuffer.GetAddressOf(), &stride, &offset);
        //m_d3d11Context->IASetVertexBuffers(1, 1, m_modelNormalBuffer.GetAddressOf(), &stride, &offset);
        //m_d3d11Context->IASetVertexBuffers(2, 1, m_modelTextureBuffer.GetAddressOf(), &stride, &offset);
        m_d3d11Context->IASetIndexBuffer(m_modelIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_d3d11Context->IASetInputLayout(m_modelInputLayout.Get());
        m_d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_d3d11Context->VSSetShader(m_modelVertexShader.Get(), nullptr, 0);
        m_d3d11Context->VSSetConstantBuffers(0, 1, m_modelConstantBuffer.GetAddressOf());

        m_d3d11Context->PSSetShader(m_modelPixelShader.Get(), nullptr, 0);
        m_d3d11Context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        m_d3d11Context->DrawIndexed(m_indexCount, 0, 0);
    }

    DX::ThrowIfFailed(m_dxgiSwapChain->Present(1, 0));
}

void NativeRenderer::Resize(const uint32_t width, const uint32_t height)
{
    if (width == 0 || height == 0 || 
        width == m_width || height == m_height)
    {
        return;
    }

    m_pendingResize = true;
    m_width = width;
    m_height = height;

    this->ResizeSwapChain();
}

void NativeRenderer::InitD3D11()
{
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL fl{};

    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, _countof(levels),
        D3D11_SDK_VERSION,
        m_d3d11Device.ReleaseAndGetAddressOf(),
        &fl,
        m_d3d11Context.ReleaseAndGetAddressOf()));

    this->CreateSwapChainForPanel();
    this->CreateBackBufferResources();

    // Sampler for background
    D3D11_SAMPLER_DESC sampDesc {};
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(m_d3d11Device->CreateSamplerState(&sampDesc, m_sampler.ReleaseAndGetAddressOf()));
}

void NativeRenderer::CreateSwapChainForPanel()
{
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    DX::ThrowIfFailed(m_d3d11Device.As(&dxgiDevice));

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    DX::ThrowIfFailed(dxgiDevice->GetAdapter(adapter.ReleaseAndGetAddressOf()));

    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    DX::ThrowIfFailed(adapter->GetParent(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    DX::ThrowIfFailed(factory->CreateSwapChainForComposition(
        m_d3d11Device.Get(), &desc, nullptr, m_dxgiSwapChain.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_nativePanel->SetSwapChain(m_dxgiSwapChain.Get()));
}

void NativeRenderer::CreateBackBufferResources()
{
    m_renderTargetView.Reset();

    Microsoft::WRL::ComPtr<ID3D11Texture2D> bb;
    DX::ThrowIfFailed(m_dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(bb.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_d3d11Device->CreateRenderTargetView(bb.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));
}

void NativeRenderer::ResizeSwapChain()
{
    if (!m_dxgiSwapChain) 
        return;

    // release resources that own backbuffer
    m_renderTargetView.Reset();

    DX::ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(
        0, // keep buffer count
        m_width, 
        m_height,
        DXGI_FORMAT_UNKNOWN, // keep format
        0));

    this->CreateBackBufferResources();
}

void NativeRenderer::CreateBackgroundPipeline()
{
    // Fullscreen triangle (SV_VertexID)
    const char* vs = R"(
            struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };
            VSOut main(uint id : SV_VertexID)
            {
                float2 pos[3] = { float2(-1,-1), float2(-1,3), float2(3,-1) };
                float2 uv[3]  = { float2(0,1),   float2(0,-1), float2(2,1)   };
                VSOut o;
                o.pos = float4(pos[id], 0, 1);
                o.uv  = uv[id];
                return o;
            }
        )";

    const char* ps = R"(
            Texture2D t0 : register(t0);
            SamplerState s0 : register(s0);
            float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target
            {
                return t0.Sample(s0, uv);
            }
        )";

    auto vsCode = DX::CompileHlsl(vs, "main", "vs_5_0");
    auto psCode = DX::CompileHlsl(ps, "main", "ps_5_0");

    DX::ThrowIfFailed(m_d3d11Device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(),
        nullptr, m_bgVertexShader.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_d3d11Device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(),
        nullptr, m_bgPixelShader.ReleaseAndGetAddressOf()));
}

void NativeRenderer::CreateModelPipeline()
{
    const char* vs = R"(
            cbuffer MVPConstantBuffer : register(b0)
            {
	            matrix model;
	            matrix view;
	            matrix projection;
            };

            struct VsInput
            {
	            float3 pos : POSITION;
	            float3 normal : NORMAL0;
	            float2 tex : TEXCOORD0;
            };

            struct PsInput
            {
	            float4 pos : SV_POSITION;
	            float3 normal : NORMAL0;
	            float2 tex : TEXCOORD0;
            };

            PsInput main(VsInput input)
            {
	            PsInput output;

	            float4 pos = float4(input.pos, 1.0f);
	            float3 normal = input.normal;

	            pos = mul(pos, model);
	            pos = mul(pos, view);
	            pos = mul(pos, projection);

	            output.pos = pos;
	            output.normal = normal;
	            output.tex = input.tex;

	            return output;
            }
        )";

    const char* ps = R"(
            Texture2D tex : register(t0);
            SamplerState texSampler : register(s0);

            cbuffer Cbuffer0 : register(b0) 
            {
	            matrix ColorMtrx;
            };

            struct PsInput 
            {
	            float4 pos : SV_POSITION;
	            float3 normal : NORMAL0;
	            float2 tex : TEXCOORD0;
            };

            float4 main(PsInput input) : SV_TARGET
            {
	            float4 color = tex.Sample(texSampler, input.tex);

	            return color;
            }
        )";

    auto vsCode = DX::CompileHlsl(vs, "main", "vs_5_0");
    auto psCode = DX::CompileHlsl(ps, "main", "ps_5_0");

    DX::ThrowIfFailed(m_d3d11Device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(),
        nullptr, m_modelVertexShader.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_d3d11Device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(),
        nullptr, m_modelPixelShader.ReleaseAndGetAddressOf()));

    D3D11_INPUT_ELEMENT_DESC modelInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    auto numElements = sizeof(modelInputLayout) / sizeof(modelInputLayout[0]);

    DX::ThrowIfFailed(m_d3d11Device->CreateInputLayout(
        modelInputLayout, numElements,
        vsCode->GetBufferPointer(), vsCode->GetBufferSize(),
        m_modelInputLayout.ReleaseAndGetAddressOf()));

    m_d3d11Context->IASetInputLayout(m_modelInputLayout.Get());
    m_d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_BUFFER_DESC constBuff { 0 };
    constBuff.Usage = D3D11_USAGE_DEFAULT;
    constBuff.ByteWidth = sizeof(DX::ConstantBufferData);
    constBuff.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constBuff.CPUAccessFlags = 0;
    constBuff.MiscFlags = 0;

    DX::ThrowIfFailed(m_d3d11Device->CreateBuffer(&constBuff, nullptr, m_modelConstantBuffer.ReleaseAndGetAddressOf()));

    auto view = DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), DirectX::XMVectorSet(0, 0, 10, 0), DirectX::g_XMIdentityR1);

    DirectX::XMStoreFloat4x4(&m_modelCBData.model, DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(0, 0, 2)));
    DirectX::XMStoreFloat4x4(&m_modelCBData.view, DirectX::XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&m_modelCBData.projection, DirectX::XMMatrixIdentity());
}

void NativeRenderer::LoadBackgroundTexture(const std::vector<uint8_t>& texData)
{
    if (texData.empty())
        return;

    // Load image
    DirectX::ScratchImage img;

    HRESULT hr = LoadFromDDSMemory(texData.data(), texData.size(), DirectX::DDS_FLAGS::DDS_FLAGS_FORCE_RGB, nullptr, img);

    if (FAILED(hr))
        hr = LoadFromTGAMemory(texData.data(), texData.size(), DirectX::TGA_FLAGS::TGA_FLAGS_NONE, nullptr, img);

    if (FAILED(hr))
        hr = LoadFromWICMemory(texData.data(), texData.size(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_RGB, nullptr, img);

    DX::ThrowIfFailed(hr);

    // CreateTexture + SRV
    Microsoft::WRL::ComPtr<ID3D11Resource> texture;

    DX::ThrowIfFailed(DirectX::CreateTexture(
        m_d3d11Device.Get(),
        img.GetImages(), 
        img.GetImageCount(),
        img.GetMetadata(),
        texture.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(DirectX::CreateShaderResourceView(
        m_d3d11Device.Get(),
        img.GetImages(),
        img.GetImageCount(),
        img.GetMetadata(),
        m_bgShaderResourceView.ReleaseAndGetAddressOf()));
}

void NativeRenderer::LoadModelAssimp(const std::vector<uint8_t>& modelData)
{
    if (modelData.empty())
        return;

    Assimp::Importer importer;
    unsigned flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals;
    const aiScene* scene = importer.ReadFileFromMemory(modelData.data(), modelData.size(), flags);

    if (!scene || !scene->mRootNode) 
        throw std::runtime_error(importer.GetErrorString());

    if (!scene->HasMeshes())
        throw std::runtime_error("Assimp error: no meshes!");

    aiMesh* mesh = scene->mMeshes[0];

    if (!mesh || !mesh->HasPositions())
        throw std::runtime_error("Assimp error: mesh doesn't exist or has no positions!");

    if (mesh->HasFaces())
    {
        m_indexCount = (uint32_t)mesh->mNumFaces * 3;
        m_modelMesh.Indices.resize(m_indexCount);

        const auto faces = mesh->mFaces;

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            if (faces && faces[i].mNumIndices == 3) 
            {
                for (int j = 0; j < 3; ++j)
                {
                    m_modelMesh.Indices[i * 3 + j] = faces[i].mIndices[j];
                }
            }
            else 
            {
                throw std::runtime_error("Assimp error: incorrect indices count!");
            }
        }
    }
    else
    {
        throw std::runtime_error("Assimp error: empty indices!");
    }

    if (mesh->HasPositions())
    {
        uint32_t vertexCount = (uint32_t)mesh->mNumVertices;

        m_modelMesh.Vertices.resize(vertexCount);

        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            m_modelMesh.Vertices[i].Position = DirectX::XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        }

        if (mesh->HasNormals())
        {
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                DirectX::XMVECTOR xvNormal = DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&mesh->mNormals[i]);
                xvNormal = DirectX::XMVector3Normalize(xvNormal);

                DirectX::XMStoreFloat3(&m_modelMesh.Vertices[i].Normal, xvNormal);
            }
        }

        if (mesh->HasTextureCoords(0))
        {
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                m_modelMesh.Vertices[i].TexCoord = DirectX::XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
        }
    }

    const size_t ibSize = m_modelMesh.Indices.size() * sizeof(uint32_t);
    const size_t vbSize = m_modelMesh.Vertices.size() * sizeof(DX::Vertex);

    // Create Index buffer
    D3D11_BUFFER_DESC indexBufferDesc { 0 };
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.ByteWidth = (UINT)ibSize;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA indexInitData { 0 };
    indexInitData.pSysMem = m_modelMesh.Indices.data();

    DX::ThrowIfFailed(m_d3d11Device->CreateBuffer(&indexBufferDesc, &indexInitData, m_modelIndexBuffer.ReleaseAndGetAddressOf()));

    // Create Vertex Buffer
    D3D11_BUFFER_DESC vertexBufferDesc { 0 };
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.ByteWidth = (UINT)vbSize;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexInitData { 0 };
    vertexInitData.pSysMem = m_modelMesh.Vertices.data();

    DX::ThrowIfFailed(m_d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexInitData, m_modelVertexBuffer.ReleaseAndGetAddressOf()));
}
