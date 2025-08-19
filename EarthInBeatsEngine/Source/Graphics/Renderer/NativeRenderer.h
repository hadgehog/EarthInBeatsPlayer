#pragma once

#include <memory>
#include <windows.h>
#include <vector>
#include <string>
#include <wrl.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <windows.ui.xaml.media.dxinterop.h>  // ISwapChainPanelNative

#include "../Scene/Model.h"
#include "../Scene/Camera.h"
#include "../Scene/TextureManager.h"
#include "UploadContext.h"

// TODO: add error text everywhere in ThrowIfFailed

class NativeRenderer
{
public:
    NativeRenderer();
    virtual ~NativeRenderer();

    bool Initialize(
        IUnknown* panel,
        uint32_t width, uint32_t height,
        const std::string& modelPath,
        const std::string& backgroundTexture = "",
        const std::string& modelTextureOverride = "");

    void Resize(uint32_t width, uint32_t height);

    void Draw();  // draws background + model
    void Shutdown();

    // Input surface
    void SetRotation(float yaw, float pitch);
    void SetZoom(float delta);
    void OnMouseDrag(float dx, float dy);
    void OnMouseWheel(int delta, bool ctrlDown);
    void OnKeyDown(WPARAM key);

    Camera& GetCamera();

    void LoadModel(const std::string& path);
    void LoadTexture(const std::string& path);
    void LoadBackground(const std::string& path);

protected:
    void BeginFrame();
    void EndFrame();

private:
    bool CreateDeviceAndSwapchain(IUnknown* panel, uint32_t w, uint32_t h);
    void CreateRenderTargetView(uint32_t w, uint32_t h);
    void CreatePipelineStates();
    void CreateConstantBuffers();
    UploadContext& GetUploadContext();

    void DrawBackground();
    void DrawModel();

private:
    static constexpr uint32_t kFrameCount = 2;
   
    // Core D3D
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

    HANDLE m_fenceEvent = 0;
    uint64_t m_fenceValue = 0;
    uint32_t m_frameIndex = 0;

    // Descriptors
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap; // shader-visible
    UINT m_rtvDescriptorSize = 0;
    UINT m_srvDescriptorSize = 0;

    // Render targets
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[kFrameCount];

    // Root/PSO
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;     // model
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bgPso;   // background

    // CBs
    struct CameraCB { DirectX::XMFLOAT4X4 viewProj; };
    struct ObjectCB { DirectX::XMFLOAT4X4 model; };

    Microsoft::WRL::ComPtr<ID3D12Resource> m_cameraCB;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_objectCB;

    // Scene objects
    TextureManager m_textureMgr;  // model textures
    TextureHandle m_bgTexture;    // background
    Model m_model;

    // State
    Camera m_camera;
    uint32_t m_width = 1;
    uint32_t m_height = 1;

    std::string m_modelPath;
    std::string m_backgroundTexture;
    std::string m_modelTextureOverride;
};
