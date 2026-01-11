#pragma once

#include <memory>
#include <vector>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <winrt/base.h>
#include <windows.ui.xaml.media.dxinterop.h>  // ISwapChainPanelNative

//#include "../Scene/Model.h"
//#include "../Scene/Camera.h"
//#include "../Scene/TextureManager.h"
//#include "UploadContext.h"

// TODO: add error text everywhere in ThrowIfFailed
// TODO: add the namespace

class NativeRenderer
{
public:
    NativeRenderer() = default;
    virtual ~NativeRenderer();

    void Initialize(
        const winrt::com_ptr<ISwapChainPanelNative>& panel,
        uint32_t width,
        uint32_t height,
        const std::vector<uint8_t>& model,
        const std::vector<uint8_t>& bgTex);

    void RenderFrame();

private:
    void InitD3D12();
    void CreateSwapChainForPanel();
    void CreateRTVs();
    void CreateSrvHeap();

    void CreateBackgroundPipeline();
    void CreateModelPipeline();

    void LoadBackgroundTexture(const std::vector<uint8_t>& texData);
    void LoadModelAssimp(const std::vector<uint8_t>& modelData);

    void CreateBuffer_DefaultWithUpload(
        size_t byteSize,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES finalState,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outDefault,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outUpload);

    void WaitForGpu();
    void MoveToNextFrame();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //bool Initialize(
    //    const winrt::com_ptr<ISwapChainPanelNative>& panel,
    //    uint32_t width, uint32_t height,
    //    const std::vector<uint8_t>& model,
    //    const std::vector<uint8_t>& backgroundTexture,
    //    const std::vector<uint8_t>& modelTextureOverride);
    //
    //void Resize(uint32_t width, uint32_t height);
    //
    //void Draw();  // draws background + model
    //void Shutdown();
    //
    //// Input surface
    //void SetRotation(float yaw, float pitch);
    //void SetZoom(float delta);
    //void OnMouseDrag(float dx, float dy);
    //void OnMouseWheel(int delta, bool ctrlDown);
    //void OnKeyDown(WPARAM key);
    //
    //Camera& GetCamera();
    //
    // TODO: implement
    //void LoadModel(const std::string& path);
    //void LoadTexture(const std::string& path);
    //void LoadBackground(const std::string& path);
    //
    //protected:
    //    void BeginFrame();
    //    void EndFrame();
    //
    //private:
    //    bool CreateDeviceAndSwapchain();
    //    void CreateRenderTargetView(uint32_t w, uint32_t h);
    //    void CreatePipelineStates();
    //    void CreateConstantBuffers();
    //    UploadContext& GetUploadContext();
    //
    //    void DrawBackground();
    //    void DrawModel();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    static constexpr uint32_t kFrameCount = 2;

    winrt::com_ptr<ISwapChainPanelNative> m_panel { nullptr };
   
    // Core D3D
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;
    uint32_t m_frameIndex = 0;

    // Descriptors
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize = 0;

    // Render targets
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[kFrameCount];

    // Sync
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    uint64_t m_fenceValue = 0;

    // Viewport
    uint32_t m_width = 1920;
    uint32_t m_height = 1200;

    // Background
    Microsoft::WRL::ComPtr<ID3D12Resource> m_bgTexture;      // DEFAULT heap
    Microsoft::WRL::ComPtr<ID3D12Resource> m_bgUpload;       // UPLOAD heap
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;  // CBV/SRV/UAV

    // Root+PSO
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_bgRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bgPso;

    // Model
    struct Vertex
    {
        float px, py, pz;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> m_vbDefault;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vbUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ibDefault;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ibUpload;

    D3D12_VERTEX_BUFFER_VIEW m_vbView{};
    D3D12_INDEX_BUFFER_VIEW  m_ibView{};
    UINT m_indexCount = 0;

    // Constant buffer (upload)
    Microsoft::WRL::ComPtr<ID3D12Resource> m_cbUpload;
    uint8_t* m_cbMapped = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_modelRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_modelPso;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //// CBs
    //struct CameraCB { DirectX::XMFLOAT4X4 viewProj; };
    //struct ObjectCB { DirectX::XMFLOAT4X4 model; };
    //
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_cameraCB;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_objectCB;
    //
    //// Scene objects
    //TextureManager m_textureMgr;  // model textures
    //TextureHandle m_bgTexture;    // background
    //Model m_model;
    //
    //// State
    //Camera m_camera;
};
