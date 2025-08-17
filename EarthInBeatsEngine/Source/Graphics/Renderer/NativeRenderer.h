#pragma once

#include <memory>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <thread>
#include <mutex>
#include <windows.ui.xaml.media.dxinterop.h>
#include <winrt/Windows.UI.Xaml.Controls.h>

#include "../Scene/Model.h"
#include "../Scene/Camera.h"

class NativeRenderer
{
public:
    NativeRenderer();
    virtual ~NativeRenderer();

    void Initialize(const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel);
    void Render();
    void Update(const float timer);
    void Shutdown();

	void LoadModel(const std::string& path);
	void LoadTexture(const std::string& path);
    void LoadBackground(const std::string& path);

	void SetRotation(float yaw, float pitch);
    void SetZoom(float delta);

private:
    void InitDevice();
    void RenderLoop();
    void DrawFrame();

private:
    static constexpr uint32_t kFrameCount = 2;
    static constexpr UINT kSrvBackground = 0;
    static constexpr UINT kSrvModel = 1;

    // Core D3D
    Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAlloc[kFrameCount];

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
    Microsoft::WRL::ComPtr<ID3D12Resource> m_cameraCB;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_objectCB;

    // Models
    std::unique_ptr<Model> m_model;

    // Textures
    //std::unique_ptr<Texture> m_modelTexture;
    //std::unique_ptr<Texture> m_backgroundTexture;
    bool m_hasBackground = false;

    // State 
    std::unique_ptr<Camera> m_camera;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float scale = 1.0f;
};

