#include "NativeRenderer.h"

#include "../Helpers/DxHelper.h"
#include "../Helpers/d3dx12.h"

#include <d3dcompiler.h>

#include <DirectXTex.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static Microsoft::WRL::ComPtr<ID3DBlob> CompileHlsl(
    const char* source,
    const char* entry,
    const char* target)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> code;
    Microsoft::WRL::ComPtr<ID3DBlob> err;

    HRESULT hr = D3DCompile(
        source, std::strlen(source),
        nullptr, nullptr, nullptr,
        entry, target, flags, 0,
        code.ReleaseAndGetAddressOf(),
        err.ReleaseAndGetAddressOf());

    if (FAILED(hr))
    {
        std::string msg = "D3DCompile failed";
        if (err) msg += ": " + std::string((const char*)err->GetBufferPointer(), err->GetBufferSize());
        throw std::runtime_error(msg);
    }

    return code;
}

NativeRenderer::~NativeRenderer()
{
	if (m_cbMapped)
	{
		m_cbUpload->Unmap(0, nullptr);
		m_cbMapped = nullptr;
	}

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

void NativeRenderer::Initialize(
	const winrt::com_ptr<ISwapChainPanelNative>& panel,
    uint32_t width,
    uint32_t height,
	const std::vector<uint8_t>& model, 
	const std::vector<uint8_t>& bgTex)
{
	m_panel = panel;
    m_width = width;
    m_height = height;

	this->InitD3D12();

	this->CreateBackgroundPipeline();
	this->CreateModelPipeline();

	this->LoadBackgroundTexture(bgTex);
	this->LoadModelAssimp(model);

	struct alignas(256) CB
	{
		float mvp[16];
	} cb{};

	cb.mvp[0] = 1; 
    cb.mvp[5] = 1; 
    cb.mvp[10] = 1; 
    cb.mvp[15] = 1;

	std::memcpy(m_cbMapped, &cb, sizeof(cb));

	RenderFrame();
}

void NativeRenderer::RenderFrame()
{
    DX::ThrowIfFailed(m_cmdAllocator[m_frameIndex]->Reset());
    DX::ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator[m_frameIndex].Get(), nullptr));

    // Transition to RT
    auto toRT = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    m_cmdList->ResourceBarrier(1, &toRT);

    // RTV handle
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += (SIZE_T)m_frameIndex * (SIZE_T)m_rtvDescriptorSize;

    // viewport/scissor
    D3D12_VIEWPORT vp{};
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MaxDepth = 1.0f;

    D3D12_RECT sc{ 0,0,(LONG)m_width,(LONG)m_height };

    m_cmdList->RSSetViewports(1, &vp);
    m_cmdList->RSSetScissorRects(1, &sc);
    m_cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    float clear[4] = { 0.05f, 0.05f, 0.08f, 1.0f };
    m_cmdList->ClearRenderTargetView(rtv, clear, 0, nullptr);

    // ---------- Draw background (fullscreen triangle) ----------
    {
        ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
        m_cmdList->SetDescriptorHeaps(1, heaps);

        m_cmdList->SetPipelineState(m_bgPso.Get());
        m_cmdList->SetGraphicsRootSignature(m_bgRootSig.Get());

        m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // root param 0: descriptor table -> srv heap start
        m_cmdList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

        m_cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // ---------- Draw model ----------
    if (m_vbDefault && m_ibDefault && m_indexCount > 0)
    {
        m_cmdList->SetPipelineState(m_modelPso.Get());
        m_cmdList->SetGraphicsRootSignature(m_modelRootSig.Get());

        m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_cmdList->IASetVertexBuffers(0, 1, &m_vbView);
        m_cmdList->IASetIndexBuffer(&m_ibView);

        // root param 0: CBV
        m_cmdList->SetGraphicsRootConstantBufferView(0, m_cbUpload->GetGPUVirtualAddress());

        m_cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
    }

    // Back to present
    auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);

    m_cmdList->ResourceBarrier(1, &toPresent);

    DX::ThrowIfFailed(m_cmdList->Close());
    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(1, lists);

    DX::ThrowIfFailed(m_swapChain->Present(1, 0));
    this->MoveToNextFrame();
}

void NativeRenderer::Resize(uint32_t width, uint32_t height)
{
    if (!m_device)
    {
        return;
    }

    if (!width || !height)
    {
        return;
    }

    m_width = width; 
    m_height = height;

    for (uint32_t i = 0; i < kFrameCount; ++i)
    {
        m_renderTargets[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC desc {}; 
    m_swapChain->GetDesc(&desc);

    DX::ThrowIfFailed(m_swapChain->ResizeBuffers(kFrameCount, m_width, m_height, desc.BufferDesc.Format, desc.Flags));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    this->CreateRTVs();
}

void NativeRenderer::InitD3D12()
{
    DX::ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf())));

    D3D12_COMMAND_QUEUE_DESC q{};
    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    DX::ThrowIfFailed(m_device->CreateCommandQueue(&q, IID_PPV_ARGS(m_cmdQueue.ReleaseAndGetAddressOf())));

    for (UINT i = 0; i < kFrameCount; ++i)
    {
        DX::ThrowIfFailed(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(m_cmdAllocator[i].ReleaseAndGetAddressOf())));
    }

    DX::ThrowIfFailed(m_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_cmdAllocator[0].Get(), nullptr,
        IID_PPV_ARGS(m_cmdList.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_cmdList->Close());

    DX::ThrowIfFailed(m_device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));

    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    if (!m_fenceEvent) 
        throw std::runtime_error("CreateEvent failed");

    this->CreateSwapChainForPanel();
    this->CreateRTVs();
    this->CreateSrvHeap();

    // Constant buffer upload (256-aligned)
    const UINT cbSize = 256;
    DX::ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(cbSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_cbUpload.ReleaseAndGetAddressOf())));

    DX::ThrowIfFailed(m_cbUpload->Map(0, nullptr, reinterpret_cast<void**>(&m_cbMapped)));
    std::memset(m_cbMapped, 0, cbSize);
}

void NativeRenderer::CreateSwapChainForPanel()
{
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = kFrameCount;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1;
    DX::ThrowIfFailed(factory->CreateSwapChainForComposition(
        m_cmdQueue.Get(), &desc, nullptr, sc1.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_panel->SetSwapChain(sc1.Get()));

    DX::ThrowIfFailed(sc1.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void NativeRenderer::CreateRTVs()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap{};
    heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap.NumDescriptors = kFrameCount;
    heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto h = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kFrameCount; ++i)
    {
        DX::ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].ReleaseAndGetAddressOf())));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, h);
        h.ptr += m_rtvDescriptorSize;
    }
}

void NativeRenderer::CreateSrvHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap{};
    heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap.NumDescriptors = 1; // background SRV only
    heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(m_srvHeap.ReleaseAndGetAddressOf())));
}

void NativeRenderer::CreateBackgroundPipeline()
{
    // Root: descriptor table (t0) + static sampler
    CD3DX12_DESCRIPTOR_RANGE range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

    CD3DX12_ROOT_PARAMETER rp[1];
    rp[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samp(
        0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rp), rp, 1, &samp,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> sig;
    Microsoft::WRL::ComPtr<ID3DBlob> err;

    DX::ThrowIfFailed(D3D12SerializeRootSignature(
        &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        sig.ReleaseAndGetAddressOf(),
        err.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_device->CreateRootSignature(
        0, sig->GetBufferPointer(), sig->GetBufferSize(),
        IID_PPV_ARGS(m_bgRootSig.ReleaseAndGetAddressOf())));

    // Fullscreen triangle (no VB)
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

    auto vsCode = CompileHlsl(vs, "main", "vs_5_0");
    auto psCode = CompileHlsl(ps, "main", "ps_5_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = m_bgRootSig.Get();
    pso.VS = { vsCode->GetBufferPointer(), vsCode->GetBufferSize() };
    pso.PS = { psCode->GetBufferPointer(), psCode->GetBufferSize() };
    pso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso.DepthStencilState.DepthEnable = FALSE;
    pso.DepthStencilState.StencilEnable = FALSE;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    pso.SampleDesc.Count = 1;

    DX::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(m_bgPso.ReleaseAndGetAddressOf())));
}

void NativeRenderer::CreateModelPipeline()
{
    // Root: CBV b0
    CD3DX12_ROOT_PARAMETER rp[1];
    rp[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init(_countof(rp), rp, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> sig;
    Microsoft::WRL::ComPtr<ID3DBlob> err;

    DX::ThrowIfFailed(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        sig.ReleaseAndGetAddressOf(),
        err.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(m_device->CreateRootSignature(
        0, sig->GetBufferPointer(), sig->GetBufferSize(),
        IID_PPV_ARGS(m_modelRootSig.ReleaseAndGetAddressOf())));

    const char* vs = R"(
            cbuffer CB : register(b0) { float4x4 mvp; };
            struct VSIn { float3 pos : POSITION; };
            struct VSOut { float4 pos : SV_Position; };
            VSOut main(VSIn i)
            {
                VSOut o;
                o.pos = mul(mvp, float4(i.pos, 1));
                return o;
            }
        )";

    const char* ps = R"(
            float4 main() : SV_Target
            {
                return float4(1,1,1,1);
            }
        )";

    auto vsCode = CompileHlsl(vs, "main", "vs_5_0");
    auto psCode = CompileHlsl(ps, "main", "ps_5_0");

    D3D12_INPUT_ELEMENT_DESC il[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = m_modelRootSig.Get();
    pso.VS = { vsCode->GetBufferPointer(), vsCode->GetBufferSize() };
    pso.PS = { psCode->GetBufferPointer(), psCode->GetBufferSize() };
    pso.InputLayout = { il, _countof(il) };
    pso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso.DepthStencilState.DepthEnable = FALSE;
    pso.DepthStencilState.StencilEnable = FALSE;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    pso.SampleDesc.Count = 1;

    DX::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(m_modelPso.ReleaseAndGetAddressOf())));
}

void NativeRenderer::LoadBackgroundTexture(const std::vector<uint8_t>& texData)
{
    if (texData.empty())
        return;

    DirectX::ScratchImage img;

    HRESULT hr = LoadFromDDSMemory(texData.data(), texData.size(), DirectX::DDS_FLAGS::DDS_FLAGS_FORCE_RGB, nullptr, img);

    if (FAILED(hr))
        hr = LoadFromTGAMemory(texData.data(), texData.size(), DirectX::TGA_FLAGS::TGA_FLAGS_NONE, nullptr, img);

    if (FAILED(hr))
        hr = LoadFromWICMemory(texData.data(), texData.size(), DirectX::WIC_FLAGS::WIC_FLAGS_FORCE_RGB, nullptr, img);

    DX::ThrowIfFailed(hr);

    const DirectX::Image* src = img.GetImage(0, 0, 0);

    if (!src) 
        throw std::runtime_error("DirectXTex: no image");

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = (UINT64)src->width;
    texDesc.Height = (UINT)src->height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = (DXGI_FORMAT)src->format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    DX::ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_bgTexture.ReleaseAndGetAddressOf())));

    UINT64 uploadSize = 0;
    m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadSize);

    DX::ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_bgUpload.ReleaseAndGetAddressOf())));

    D3D12_SUBRESOURCE_DATA sub{};
    sub.pData = src->pixels;
    sub.RowPitch = (LONG_PTR)src->rowPitch;
    sub.SlicePitch = (LONG_PTR)src->slicePitch;

    // Record
    DX::ThrowIfFailed(m_cmdAllocator[m_frameIndex]->Reset());
    DX::ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator[m_frameIndex].Get(), nullptr));

    UpdateSubresources(m_cmdList.Get(), m_bgTexture.Get(), m_bgUpload.Get(), 0, 0, 1, &sub);
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_bgTexture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cmdList->ResourceBarrier(1, &barrier);

    DX::ThrowIfFailed(m_cmdList->Close());
    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(1, lists);
    this->WaitForGpu();

    // Create SRV in heap[0]
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = texDesc.Format;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(m_bgTexture.Get(), &srv, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
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
    if (scene->mNumMeshes == 0) 
        throw std::runtime_error("Assimp: no meshes");

    aiMesh* mesh = scene->mMeshes[0];
    if (!mesh->HasPositions()) 
        throw std::runtime_error("Assimp: mesh has no positions");

    std::vector<Vertex> verts;
    verts.reserve(mesh->mNumVertices);

    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        auto& p = mesh->mVertices[i];
        verts.push_back(Vertex{ p.x, p.y, p.z });
    }

    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3);

    for (unsigned f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices == 3)
        {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

    m_indexCount = (UINT)indices.size();
    if (m_indexCount == 0) 
        throw std::runtime_error("Assimp: empty indices");

    const size_t vbSize = verts.size() * sizeof(Vertex);
    const size_t ibSize = indices.size() * sizeof(uint32_t);

    this->CreateBuffer_DefaultWithUpload(vbSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        m_vbDefault, m_vbUpload);
    this->CreateBuffer_DefaultWithUpload(ibSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_INDEX_BUFFER,
        m_ibDefault, m_ibUpload);

    // Upload data
    D3D12_SUBRESOURCE_DATA vbSub{};
    vbSub.pData = verts.data();
    vbSub.RowPitch = (LONG_PTR)vbSize;
    vbSub.SlicePitch = vbSub.RowPitch;

    D3D12_SUBRESOURCE_DATA ibSub{};
    ibSub.pData = indices.data();
    ibSub.RowPitch = (LONG_PTR)ibSize;
    ibSub.SlicePitch = ibSub.RowPitch;

    DX::ThrowIfFailed(m_cmdAllocator[m_frameIndex]->Reset());
    DX::ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator[m_frameIndex].Get(), nullptr));

    UpdateSubresources(m_cmdList.Get(), m_vbDefault.Get(), m_vbUpload.Get(), 0, 0, 1, &vbSub);
    UpdateSubresources(m_cmdList.Get(), m_ibDefault.Get(), m_ibUpload.Get(), 0, 0, 1, &ibSub);

    auto vbBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vbDefault.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    auto ibBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_ibDefault.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    D3D12_RESOURCE_BARRIER bars[] = { vbBarrier, ibBarrier };
    m_cmdList->ResourceBarrier(_countof(bars), bars);

    DX::ThrowIfFailed(m_cmdList->Close());
    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(1, lists);
    this->WaitForGpu();

    // Views
    m_vbView.BufferLocation = m_vbDefault->GetGPUVirtualAddress();
    m_vbView.StrideInBytes = sizeof(Vertex);
    m_vbView.SizeInBytes = (UINT)vbSize;

    m_ibView.BufferLocation = m_ibDefault->GetGPUVirtualAddress();
    m_ibView.Format = DXGI_FORMAT_R32_UINT;
    m_ibView.SizeInBytes = (UINT)ibSize;
}

void NativeRenderer::CreateBuffer_DefaultWithUpload(
	size_t byteSize, 
	D3D12_RESOURCE_FLAGS flags, 
	D3D12_RESOURCE_STATES finalState, 
	Microsoft::WRL::ComPtr<ID3D12Resource>& outDefault, 
	Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload)
{
    DX::ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize, flags),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(outDefault.ReleaseAndGetAddressOf())));

    DX::ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(outUpload.ReleaseAndGetAddressOf())));
}

void NativeRenderer::WaitForGpu()
{
    const UINT64 fenceToWait = m_fenceValue++;
    DX::ThrowIfFailed(m_cmdQueue->Signal(m_fence.Get(), fenceToWait));

    if (m_fence->GetCompletedValue() < fenceToWait)
    {
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWait, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void NativeRenderer::MoveToNextFrame()
{
    const UINT64 fenceToSignal = m_fenceValue++;
    DX::ThrowIfFailed(m_cmdQueue->Signal(m_fence.Get(), fenceToSignal));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    if (m_fence->GetCompletedValue() < fenceToSignal)
    {
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToSignal, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

//bool NativeRenderer::Initialize(
//    const winrt::com_ptr<ISwapChainPanelNative>& panel,
//	uint32_t width, 
//	uint32_t height, 
//	const std::vector<uint8_t>& model, 
//	const std::vector<uint8_t>& backgroundTexture, 
//	const std::vector<uint8_t>& modelTextureOverride)
//{
//    m_panel = panel;
//
//    m_width = width; 
//    m_height = height;
//
//    if (!this->CreateDeviceAndSwapchain())
//    {
//        return false;
//    }
//
//    this->CreateRenderTargetView(width, height);
//    this->CreatePipelineStates();
//
//    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvDesc {};
//    cbvSrvDesc.NumDescriptors = 1024;
//    cbvSrvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//    cbvSrvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//
//    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
//
//    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
//    this->CreateConstantBuffers();
//
//    m_camera.SetViewport((float)width, (float)height);
//    m_camera.SetDistance(5.0f);
//
//    m_textureMgr.Initialize(m_device.Get(), m_cbvSrvHeap.Get(), m_srvDescriptorSize);
//
//    UploadContext ctx = this->GetUploadContext();
//
//    // ACCESS DENIED. should read from bytes
//    if (!backgroundTexture.empty())
//    {
//        m_bgTexture = m_textureMgr.LoadTexture(ctx, backgroundTexture);
//    }
//
//    // ACCESS DENIED. should read from bytes
//    if (!model.empty())
//    {
//        m_model.Initialize(m_device.Get(), model);
//        m_model.ResolveMaterials(m_textureMgr, ctx);
//
//        if (!modelTextureOverride.empty())
//        {
//            m_model.OverrideAllBaseColor(m_textureMgr, ctx, modelTextureOverride);
//        }
//    }
//
//    this->Draw();
//
//    return true;
//}

//void NativeRenderer::Resize(uint32_t width, uint32_t height)
//{
//    if (!m_device)
//    {
//        return;
//    }
//
//    m_width = width; 
//    m_height = height;
//
//    for (uint32_t i = 0; i < kFrameCount; ++i)
//    {
//        m_renderTargets[i].Reset();
//    }
//
//    DXGI_SWAP_CHAIN_DESC desc {}; 
//    m_swapChain->GetDesc(&desc);
//
//    DX::ThrowIfFailed(m_swapChain->ResizeBuffers(kFrameCount, width, height, desc.BufferDesc.Format, desc.Flags));
//
//    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
//    this->CreateRenderTargetView(width, height);
//    m_camera.SetViewport((float)width, (float)height);
//}
//
//void NativeRenderer::Draw()
//{
//    this->BeginFrame();
//
//    this->DrawBackground();
//    this->DrawModel();
//
//    this->EndFrame();
//}
//
//void NativeRenderer::Shutdown()
//{
//    // TODO: implement
//}
//
//void NativeRenderer::SetRotation(float yaw, float pitch)
//{
//    // TODO: implement
//}
//
//void NativeRenderer::SetZoom(float delta)
//{
//    // TODO: implement
//}
//
//void NativeRenderer::OnMouseDrag(float dx, float dy)
//{
//    // TODO: implement
//}
//
//void NativeRenderer::OnMouseWheel(int delta, bool ctrlDown)
//{
//    // TODO: implement
//}
//
//void NativeRenderer::OnKeyDown(WPARAM key)
//{
//    // TODO: implement
//}
//
//Camera& NativeRenderer::GetCamera()
//{
//    return m_camera;
//}

//void NativeRenderer::LoadModel(const std::string& path)
//{
//    // TODO: implement
//}
//
//void NativeRenderer::LoadTexture(const std::string& path)
//{
//    // TODO: implement
//}
//
//void NativeRenderer::LoadBackground(const std::string& path)
//{
//    // TODO: implement
//}

//void NativeRenderer::BeginFrame()
//{
//    DX::ThrowIfFailed(m_cmdAllocator[m_frameIndex]->Reset());
//    DX::ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator[m_frameIndex].Get(), m_pso.Get()));
//
//    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
//        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
//    m_cmdList->ResourceBarrier(1, &barrier);
//
//    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
//    m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
//
//    D3D12_VIEWPORT vp { 0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f };
//    D3D12_RECT sc { 0, 0, (LONG)m_width, (LONG)m_height };
//    m_cmdList->RSSetViewports(1, &vp);
//    m_cmdList->RSSetScissorRects(1, &sc);
//
//    const float clear[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
//    m_cmdList->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);
//
//    ID3D12DescriptorHeap* heaps[] = { m_cbvSrvHeap.Get() };
//    m_cmdList->SetDescriptorHeaps(1, heaps);
//    m_cmdList->SetGraphicsRootSignature(m_rootSig.Get());
//}
//
//void NativeRenderer::EndFrame()
//{
//    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
//        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
//
//    m_cmdList->ResourceBarrier(1, &barrier);
//
//    DX::ThrowIfFailed(m_cmdList->Close());
//
//    ID3D12CommandList* lists[] = { m_cmdList.Get() };
//    m_cmdQueue->ExecuteCommandLists(1, lists);
//
//    DX::ThrowIfFailed(m_swapChain->Present(1, 0));
//
//    const uint64_t fenceToWait = ++m_fenceValue;
//
//    DX::ThrowIfFailed(m_cmdQueue->Signal(m_fence.Get(), fenceToWait));
//
//    if (m_fence->GetCompletedValue() < fenceToWait)
//    {
//        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWait, m_fenceEvent));
//        WaitForSingleObject(m_fenceEvent, INFINITE);
//    }
//
//    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
//}

//bool NativeRenderer::CreateDeviceAndSwapchain()
//{
//#ifdef _DEBUG
//    { 
//        Microsoft::WRL::ComPtr<ID3D12Debug> dbg; 
//        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) 
//            dbg->EnableDebugLayer(); 
//    }
//#endif
//
//    UINT dxgiFactoryFlags = 0;
//    Microsoft::WRL::ComPtr<IDXGIFactory4> factory; 
//
//    DX::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
//    DX::ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
//
//    D3D12_COMMAND_QUEUE_DESC qdesc {}; 
//    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//
//    DX::ThrowIfFailed(m_device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&m_cmdQueue)));
//
//    DXGI_SWAP_CHAIN_DESC1 scDesc {};
//    scDesc.Width = m_width; 
//    scDesc.Height = m_height;
//    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//    scDesc.BufferCount = kFrameCount;
//    scDesc.Scaling = DXGI_SCALING_STRETCH;
//    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
//    scDesc.SampleDesc.Count = 1;
//
//    Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1Tmp;
//    DX::ThrowIfFailed(factory->CreateSwapChainForComposition(
//        m_cmdQueue.Get(), &scDesc, nullptr, sc1Tmp.ReleaseAndGetAddressOf()
//    ));
//    DX::ThrowIfFailed(sc1Tmp.As(&m_swapChain));
//
//    //DX::ThrowIfFailed(factory->CreateSwapChainForComposition(
//    //    m_device.Get(),
//    //    &scDesc,
//    //    nullptr,
//    //    m_swapChain.ReleaseAndGetAddressOf()
//    //));
//
//    DX::ThrowIfFailed(m_panel->SetSwapChain(m_swapChain.Get()));
//
//    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
//
//    for (UINT i = 0; i < kFrameCount; ++i)
//    {
//        DX::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator[i])));
//    }
//
//    DX::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator[0].Get(), nullptr, IID_PPV_ARGS(&m_cmdList)));
//
//    m_cmdList->Close();
//
//    DX::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
//    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
//
//    if (m_fenceEvent)
//    {
//        return true;
//    }
//
//    return false;
//}
//
//void NativeRenderer::CreateRenderTargetView(uint32_t w, uint32_t h)
//{
//    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc {}; 
//
//    rtvDesc.NumDescriptors = kFrameCount; 
//    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//
//    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)));
//
//    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//
//    for (UINT i = 0; i < kFrameCount; ++i)
//    {
//        DX::ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
//        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_rtvDescriptorSize);
//        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handle);
//    }
//}
//
//void NativeRenderer::CreatePipelineStates()
//{
//    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
//#ifdef _DEBUG
//    flags |= D3DCOMPILE_DEBUG;
//#endif
//
//    auto vsModelData = DX::LoadPackageFile(L"EarthInBeatsEngine\\basic_vs.cso");
//    auto psModelData = DX::LoadPackageFile(L"EarthInBeatsEngine\\basic_ps.cso");
//    auto vsBackgroundData = DX::LoadPackageFile(L"EarthInBeatsEngine\\background_vs.cso");
//    auto psBackgroundData = DX::LoadPackageFile(L"EarthInBeatsEngine\\background_ps.cso");
//
//    CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
//    CD3DX12_ROOT_PARAMETER1 params[3];
//
//    params[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
//    params[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
//    params[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
//
//    D3D12_STATIC_SAMPLER_DESC samp {};
//
//    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
//    samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
//    samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
//    samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
//    samp.ShaderRegister = 0;
//    samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
//
//    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc {}; 
//
//    rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
//    rsDesc.Desc_1_1 = { 3, params, 1, &samp, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
//
//    Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
//    Microsoft::WRL::ComPtr<ID3DBlob> rsErr;
//
//    DX::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &rsErr));
//    DX::ThrowIfFailed(m_device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)));
//
//    D3D12_INPUT_ELEMENT_DESC layout[] = 
//    {
//        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//    };
//
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
//    // base PSO desc
//    psoDesc.pRootSignature = m_rootSig.Get();
//    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//    psoDesc.SampleMask = UINT_MAX;
//    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//    psoDesc.DepthStencilState.DepthEnable = FALSE;
//    psoDesc.DepthStencilState.StencilEnable = FALSE;
//    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//    psoDesc.NumRenderTargets = 1;
//    psoDesc.SampleDesc.Count = 1;
//
//    // model PSO desc
//    psoDesc.VS = { vsModelData.data(), vsModelData.size() };
//    psoDesc.PS = { psModelData.data(), psModelData.size() };
//    psoDesc.InputLayout = { layout, _countof(layout) };
//
//    DX::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));
//
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC bgDesc = psoDesc;
//    // background PSO desc
//    bgDesc.VS = { vsBackgroundData.data(), vsBackgroundData.size() };
//    bgDesc.PS = { psBackgroundData.data(), psBackgroundData.size() };
//    bgDesc.InputLayout = { nullptr, 0 };
//
//    DX::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&bgDesc, IID_PPV_ARGS(&m_bgPso)));
//}
//
//void NativeRenderer::CreateConstantBuffers()
//{
//    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
//    auto camDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(CameraCB));
//    auto objDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ObjectCB));
//
//    DX::ThrowIfFailed(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &camDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cameraCB)));
//    DX::ThrowIfFailed(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &objDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_objectCB)));
//}
//
//UploadContext& NativeRenderer::GetUploadContext()
//{
//    UploadContext ctx {};
//
//    ctx.m_device = m_device.Get();
//    ctx.m_queue = m_cmdQueue.Get();
//    ctx.m_alloc = m_cmdAllocator[m_frameIndex].Get();
//    ctx.m_list = m_cmdList.Get();
//    ctx.m_fence = m_fence.Get();
//    ctx.m_fenceEvent = m_fenceEvent;
//    ctx.m_fenceValue = &m_fenceValue;
//
//    return ctx;
//}
//
//void NativeRenderer::DrawBackground()
//{
//    if (!m_bgTexture.valid())
//    {
//        return;
//    }
//
//    m_cmdList->SetPipelineState(m_bgPso.Get());
//    m_cmdList->SetGraphicsRootDescriptorTable(2, m_bgTexture.m_gpu);
//    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    m_cmdList->DrawInstanced(3, 1, 0, 0);
//    m_cmdList->SetPipelineState(m_pso.Get());
//}
//
//void NativeRenderer::DrawModel()
//{
//    CameraCB cam {}; 
//
//    XMStoreFloat4x4(&cam.viewProj, XMMatrixTranspose(m_camera.ViewProj()));
//
//    void* p = nullptr; 
//    
//    m_cameraCB->Map(0, nullptr, &p); 
//    std::memcpy(p, &cam, sizeof(cam)); 
//    m_cameraCB->Unmap(0, nullptr);
//
//    ObjectCB obj {}; 
//    
//    XMStoreFloat4x4(&obj.model, XMMatrixTranspose(m_model.ModelMatrix()));
//
//    m_objectCB->Map(0, nullptr, &p); 
//    std::memcpy(p, &obj, sizeof(obj)); 
//    m_objectCB->Unmap(0, nullptr);
//
//    m_cmdList->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());
//    m_cmdList->SetGraphicsRootConstantBufferView(1, m_objectCB->GetGPUVirtualAddress());
//
//    m_cmdList->SetPipelineState(m_pso.Get());
//
//    const auto& meshes = m_model.Meshes();
//
//    for (const auto& mesh : meshes)
//    {
//        const auto& mat = m_model.MaterialAt(mesh.MaterialIndex());
//
//        if (mat.m_baseColorTexture.valid())
//        {
//            m_cmdList->SetGraphicsRootDescriptorTable(2, mat.m_baseColorTexture.m_gpu);
//        }
//
//        mesh.Draw(m_cmdList.Get());
//    }
//}
