#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <mutex>
#include <algorithm>

#include <wrl.h>
#include <winrt/base.h>
#include <windows.ui.xaml.media.dxinterop.h>  // ISwapChainPanelNative

// DirectX11
#include <d3d11_4.h>        // ID3D11Device, ID3D11DeviceContext, IDXGISwapChain1
#include <dxgi1_4.h>        // IDXGIFactory2, CreateSwapChainForComposition
#include <DirectXMath.h>    // Matrices

#include "RenderDataTypes.h"
#include "ConstantBufferData.h"

//#include "../Scene/Model.h"
//#include "../Scene/Camera.h"
//#include "../Scene/TextureManager.h"
//#include "UploadContext.h"


/// TODO:
/// 1. Add INativeRenderer interface, inherit NativeRendererDX11, NativeRendererDX12, etc... maybe even NativeRendererOpenGL 
/// 2. Add namespace GraphicsEngine for for entire renderer, and GraphicsEngine::DX for all helpers. AudioEngine for Audio (check smart pointers usage)
/// 3. Add error messages everywhere in ThrowIfFailed
/// 4. Implement engine like the scene with a list of Models and Backgrounds with Init and Draw methods, and Camera (use DirectX tutorial and OpenGL test project as referrences)


class NativeRenderer
{
public:
    NativeRenderer() = default;
    virtual ~NativeRenderer() = default;

    void Initialize(
        const winrt::com_ptr<ISwapChainPanelNative>& panel,
        const uint32_t width,
        const uint32_t height,
        const std::vector<uint8_t>& model,
        const std::vector<uint8_t>& bgTex,
        const std::vector<uint8_t>& modelTex);

    void RenderFrame();

    void Resize(const uint32_t width, const uint32_t height);

private:
    void InitD3D11();
    void CreateSwapChainForPanel();
    void CreateBackBufferResources();
    void ResizeSwapChain();

    void CreateBackgroundPipeline();
    void CreateModelPipeline();

    void LoadBackgroundTexture(const std::vector<uint8_t>& texData);
    void LoadModelAssimp(const std::vector<uint8_t>& modelData);
    void LoadModelTexture(const std::vector<uint8_t>& modelTexData);

private:
    static constexpr uint32_t kFrameCount = 2;

    // native winrt swap chain panel
    winrt::com_ptr<ISwapChainPanelNative> m_nativePanel { nullptr };

    // D3D11 core
    Microsoft::WRL::ComPtr<ID3D11Device>        m_d3d11Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3d11Context;

    // swapchain for SwapChainPanel
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_dxgiSwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;

    // background
    Microsoft::WRL::ComPtr<ID3D11VertexShader>          m_bgVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>           m_bgPixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_bgInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>                m_bgConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_bgShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>          m_sampler;

    // model buffers
    DX::MeshData m_modelMesh;   // move to Model class
    uint32_t m_indexCount = 0;  // TODO: remove

    DX::ConstantBufferData m_modelCBData;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelNormalBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_modelTextureBuffer;

    // model pipeline
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_modelVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_modelPixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_modelInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       m_modelConstantBuffer;

    // state: position, camera, resize
    std::mutex m_stateMutex;

    uint32_t m_width = 1920;
    uint32_t m_height = 1200;

    bool m_pendingResize = true;

    DirectX::XMFLOAT3 m_modelCenter { 0,0,0 };
    float m_modelScale = 1.0f;










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
