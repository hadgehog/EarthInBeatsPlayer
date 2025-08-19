#include "NativeRenderer.h"
#include "../Helpers/DxHelper.h"
#include "../Helpers/d3dx12.h"

#include <d3dcompiler.h>

NativeRenderer::NativeRenderer()
{
}

NativeRenderer::~NativeRenderer()
{
}

bool NativeRenderer::Initialize(
	IUnknown* panel, 
	uint32_t width, 
	uint32_t height, 
	const std::string& modelPath, 
	const std::string& backgroundTexture, 
	const std::string& modelTextureOverride)
{
    m_width = width; 
    m_height = height;
    m_modelPath = modelPath;
    m_backgroundTexture = backgroundTexture;
    m_modelTextureOverride = modelTextureOverride;

    if (!this->CreateDeviceAndSwapchain(panel, width, height))
    {
        return false;
    }

    this->CreateRenderTargetView(width, height);
    this->CreatePipelineStates();

    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvDesc {};
    cbvSrvDesc.NumDescriptors = 1024;
    cbvSrvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    this->CreateConstantBuffers();

    m_camera.SetViewport((float)width, (float)height);
    m_camera.SetDistance(5.0f);

    m_textureMgr.Initialize(m_device.Get(), m_cbvSrvHeap.Get(), m_srvDescriptorSize);

    UploadContext ctx = this->GetUploadContext();

    if (!m_backgroundTexture.empty())
    {
        m_bgTexture = m_textureMgr.LoadTexture(ctx, m_backgroundTexture);
    }

    if (!m_modelPath.empty())
    {
        m_model.Initialize(m_device.Get(), m_modelPath);
        m_model.ResolveMaterials(m_textureMgr, ctx);

        if (!m_modelTextureOverride.empty())
        {
            m_model.OverrideAllBaseColor(m_textureMgr, ctx, m_modelTextureOverride);
        }
    }

    return true;
}

void NativeRenderer::Resize(uint32_t width, uint32_t height)
{
    if (!m_device)
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

    DX::ThrowIfFailed(m_swapChain->ResizeBuffers(kFrameCount, width, height, desc.BufferDesc.Format, desc.Flags));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    this->CreateRenderTargetView(width, height);
    m_camera.SetViewport((float)width, (float)height);
}

void NativeRenderer::Draw()
{
    this->BeginFrame();

    this->DrawBackground();
    this->DrawModel();

    this->EndFrame();
}

void NativeRenderer::Shutdown()
{
    // TODO: implement
}

void NativeRenderer::SetRotation(float yaw, float pitch)
{
    // TODO: implement
}

void NativeRenderer::SetZoom(float delta)
{
    // TODO: implement
}

void NativeRenderer::OnMouseDrag(float dx, float dy)
{
    // TODO: implement
}

void NativeRenderer::OnMouseWheel(int delta, bool ctrlDown)
{
    // TODO: implement
}

void NativeRenderer::OnKeyDown(WPARAM key)
{
    // TODO: implement
}

Camera& NativeRenderer::GetCamera()
{
    return m_camera;
}

void NativeRenderer::LoadModel(const std::string& path)
{
    // TODO: implement
}

void NativeRenderer::LoadTexture(const std::string& path)
{
    // TODO: implement
}

void NativeRenderer::LoadBackground(const std::string& path)
{
    // TODO: implement
}

void NativeRenderer::BeginFrame()
{
    DX::ThrowIfFailed(m_cmdAllocator[m_frameIndex]->Reset());
    DX::ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator[m_frameIndex].Get(), m_pso.Get()));

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_cmdList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT vp { 0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f };
    D3D12_RECT sc { 0, 0, (LONG)m_width, (LONG)m_height };
    m_cmdList->RSSetViewports(1, &vp);
    m_cmdList->RSSetScissorRects(1, &sc);

    const float clear[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
    m_cmdList->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);

    ID3D12DescriptorHeap* heaps[] = { m_cbvSrvHeap.Get() };
    m_cmdList->SetDescriptorHeaps(1, heaps);
    m_cmdList->SetGraphicsRootSignature(m_rootSig.Get());
}

void NativeRenderer::EndFrame()
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_cmdList->ResourceBarrier(1, &barrier);

    DX::ThrowIfFailed(m_cmdList->Close());

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(1, lists);

    DX::ThrowIfFailed(m_swapChain->Present(1, 0));

    const uint64_t fenceToWait = ++m_fenceValue;

    DX::ThrowIfFailed(m_cmdQueue->Signal(m_fence.Get(), fenceToWait));

    if (m_fence->GetCompletedValue() < fenceToWait)
    {
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWait, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

bool NativeRenderer::CreateDeviceAndSwapchain(IUnknown* panel, uint32_t w, uint32_t h)
{
    UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
    { 
        Microsoft::WRL::ComPtr<ID3D12Debug> dbg; 
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) 
            dbg->EnableDebugLayer(); 
    }
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory; 

    DX::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
    DX::ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

    D3D12_COMMAND_QUEUE_DESC qdesc {}; 

    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    DX::ThrowIfFailed(m_device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&m_cmdQueue)));

    DXGI_SWAP_CHAIN_DESC1 scDesc {};

    scDesc.Width = w; scDesc.Height = h;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = kFrameCount;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    scDesc.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChainTmp;

    DX::ThrowIfFailed(factory->CreateSwapChainForComposition(m_cmdQueue.Get(), &scDesc, nullptr, &swapChainTmp));
    DX::ThrowIfFailed(swapChainTmp.As(&m_swapChain));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    Microsoft::WRL::ComPtr<ISwapChainPanelNative> panelNative;

    DX::ThrowIfFailed(panel->QueryInterface(IID_PPV_ARGS(&panelNative)));
    DX::ThrowIfFailed(panelNative->SetSwapChain(m_swapChain.Get()));

    for (UINT i = 0; i < kFrameCount; ++i)
    {
        DX::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator[i])));
    }

    DX::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator[0].Get(), nullptr, IID_PPV_ARGS(&m_cmdList)));

    m_cmdList->Close();

    DX::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return true;
}

void NativeRenderer::CreateRenderTargetView(uint32_t w, uint32_t h)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc {}; 

    rtvDesc.NumDescriptors = kFrameCount; 
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)));

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (UINT i = 0; i < kFrameCount; ++i)
    {
        DX::ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, m_rtvDescriptorSize);
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, handle);
    }
}

void NativeRenderer::CreatePipelineStates()
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> errs;
    Microsoft::WRL::ComPtr<ID3DBlob> vsModel;
    Microsoft::WRL::ComPtr<ID3DBlob> psModel;
    std::wstring shadersPath = L"C:/Users/vladi/OneDrive/My Projects/Eart_In_Beats - new/EarthInBeatsPlayer/EarthInBeatsEngine/Source/Graphics/Shaders/";

    DX::ThrowIfFailed(D3DCompileFromFile((shadersPath + L"basic_vs.hlsl").c_str(), nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsModel, &errs),
        "An error occurred while loading the basic vertex shader!");
    DX::ThrowIfFailed(D3DCompileFromFile((shadersPath + L"basic_ps.hlsl").c_str(), nullptr, nullptr, "main", "ps_5_0", flags, 0, &psModel, &errs),
        "An error occurred while loading the basic pixel shader!");

    Microsoft::WRL::ComPtr<ID3DBlob> vsBg;
    Microsoft::WRL::ComPtr<ID3DBlob> psBg;

    DX::ThrowIfFailed(D3DCompileFromFile((shadersPath + L"background_vs.hlsl").c_str(), nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsBg, &errs),
        "An error occurred while loading the background vertex shader!");
    DX::ThrowIfFailed(D3DCompileFromFile((shadersPath + L"background_ps.hlsl").c_str(), nullptr, nullptr, "main", "ps_5_0", flags, 0, &psBg, &errs),
        "An error occurred while loading the background pixel shader!");

    CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_ROOT_PARAMETER1 params[3];

    params[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
    params[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
    params[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC samp {};

    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.ShaderRegister = 0;
    samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc {}; 

    rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rsDesc.Desc_1_1 = { 3, params, 1, &samp, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };

    Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> rsErr;

    DX::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &rsErr));
    DX::ThrowIfFailed(m_device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSig)));

    D3D12_INPUT_ELEMENT_DESC layout[] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};

    psoDesc.pRootSignature = m_rootSig.Get();
    psoDesc.VS = { vsModel->GetBufferPointer(), vsModel->GetBufferSize() };
    psoDesc.PS = { psModel->GetBufferPointer(), psModel->GetBufferSize() };
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.InputLayout = { layout, _countof(layout) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1; psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    DX::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC bgDesc = psoDesc;

    bgDesc.VS = { vsBg->GetBufferPointer(), vsBg->GetBufferSize() };
    bgDesc.PS = { psBg->GetBufferPointer(), psBg->GetBufferSize() };
    bgDesc.InputLayout = { nullptr, 0 };

    DX::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&bgDesc, IID_PPV_ARGS(&m_bgPso)));
}

void NativeRenderer::CreateConstantBuffers()
{
    CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
    auto camDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(CameraCB));
    auto objDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ObjectCB));

    DX::ThrowIfFailed(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &camDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cameraCB)));
    DX::ThrowIfFailed(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &objDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_objectCB)));
}

UploadContext& NativeRenderer::GetUploadContext()
{
    UploadContext ctx {};

    ctx.m_device = m_device.Get();
    ctx.m_queue = m_cmdQueue.Get();
    ctx.m_alloc = m_cmdAllocator[m_frameIndex].Get();
    ctx.m_list = m_cmdList.Get();
    ctx.m_fence = m_fence.Get();
    ctx.m_fenceEvent = m_fenceEvent;
    ctx.m_fenceValue = &m_fenceValue;

    return ctx;
}

void NativeRenderer::DrawBackground()
{
    if (!m_bgTexture.valid())
    {
        return;
    }

    m_cmdList->SetPipelineState(m_bgPso.Get());
    m_cmdList->SetGraphicsRootDescriptorTable(2, m_bgTexture.m_gpu);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(3, 1, 0, 0);
    m_cmdList->SetPipelineState(m_pso.Get());
}

void NativeRenderer::DrawModel()
{
    CameraCB cam {}; 

    XMStoreFloat4x4(&cam.viewProj, XMMatrixTranspose(m_camera.ViewProj()));

    void* p = nullptr; 
    
    m_cameraCB->Map(0, nullptr, &p); 
    std::memcpy(p, &cam, sizeof(cam)); 
    m_cameraCB->Unmap(0, nullptr);

    ObjectCB obj {}; 
    
    XMStoreFloat4x4(&obj.model, XMMatrixTranspose(m_model.ModelMatrix()));

    m_objectCB->Map(0, nullptr, &p); 
    std::memcpy(p, &obj, sizeof(obj)); 
    m_objectCB->Unmap(0, nullptr);

    m_cmdList->SetGraphicsRootConstantBufferView(0, m_cameraCB->GetGPUVirtualAddress());
    m_cmdList->SetGraphicsRootConstantBufferView(1, m_objectCB->GetGPUVirtualAddress());

    m_cmdList->SetPipelineState(m_pso.Get());

    const auto& meshes = m_model.Meshes();

    for (const auto& mesh : meshes)
    {
        const auto& mat = m_model.MaterialAt(mesh.MaterialIndex());

        if (mat.m_baseColorTexture.valid())
        {
            m_cmdList->SetGraphicsRootDescriptorTable(2, mat.m_baseColorTexture.m_gpu);
        }

        mesh.Draw(m_cmdList.Get());
    }
}
