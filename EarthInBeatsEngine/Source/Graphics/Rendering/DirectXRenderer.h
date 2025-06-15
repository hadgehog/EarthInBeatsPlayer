#pragma once

#include "INativeRenderer.h"

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include <winrt/Windows.UI.Xaml.Controls.h>

class DirectXRenderer : public INativeRenderer
{
public:
	DirectXRenderer();
	~DirectXRenderer() override;

	void LoadModel(const std::string& path) override;
	void LoadTexture(const std::string& path) override;

	void Start() override;
	void Stop() override;

	void SetRotation(float yaw, float pitch) override;

	void Initialize(const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel);

private:
    void InitDevice();
    void RenderLoop();
    void DrawFrame();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;

    winrt::Windows::UI::Xaml::Controls::SwapChainPanel swapChainPanel { nullptr };

    std::atomic<bool> running;
    std::thread renderThread;
    std::mutex manipulationMutex;

    float yaw = 0.0f;
    float pitch = 0.0f;

    //std::unique_ptr<ModelManager> modelManager;
};

