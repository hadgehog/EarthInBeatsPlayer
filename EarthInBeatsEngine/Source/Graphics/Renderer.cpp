#include "Renderer.h"
#include "Graphics.Renderer.g.cpp"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.ApplicationModel.h>

#include <windows.ui.xaml.media.dxinterop.h>

#include "Renderer/NativeRenderer.h"

namespace winrt::EarthInBeatsEngine::Graphics::implementation
{
	Windows::Foundation::IAsyncAction Renderer::Initialize(
		const Windows::UI::Xaml::Controls::SwapChainPanel& panel,
		uint32_t width, 
		uint32_t height,
		const hstring& modelPath,
		const hstring& backgroundTexturePath,
		const hstring& modelTextureOverridePath)
	{
		std::vector<uint8_t> mainModel;
		std::vector<uint8_t> backgroundTex;
		std::vector<uint8_t> modelTexOverride;

		// fast fix for the crash
		hstring modelPathLocal(modelPath.data(), modelPath.size());
		hstring bgTexturePathLocal(backgroundTexturePath.data(), backgroundTexturePath.size());

		com_ptr<ISwapChainPanelNative> panelNative = panel.as<ISwapChainPanelNative>();

		co_await this->LoadAssetFromAppxPath(modelPathLocal, mainModel);
		co_await this->LoadAssetFromAppxPath(bgTexturePathLocal, backgroundTex);
		// co_await this->LoadAssetFromAppxPath(modelTextureOverridePath, modelTexOverride);

		m_renderer = std::make_unique<NativeRenderer>();
		m_renderer->Initialize(panelNative, width, height, mainModel, backgroundTex);

		//if (!isInitialized)
		//{
		//	throw hresult_invalid_argument(L"Native C++ Renderer could not initialize!");
		//}

		co_return;
	}

	void Renderer::Resize(uint32_t width, uint32_t height)
	{
		m_renderer->Resize(width, height);
	}

	void Renderer::Render()
	{
		m_renderer->RenderFrame();
	}

	void Renderer::Shutdown()
	{
		// TODO: implement
	}

	void Renderer::SetRotation(float yaw, float pitch)
	{
		// TODO: implement
	}

	void Renderer::SetZoom(float delta)
	{
		// TODO: implement
	}

	bool Renderer::Manipulation() const
	{
		// TODO: implement
		return false;
	}

	void Renderer::Manipulation(bool isManipulating)
	{
		// TODO: implement
	}

	Windows::Foundation::IAsyncAction Renderer::LoadAssetFromAppxPath(const hstring& path, std::vector<uint8_t>& outData)
	{
		if (path.empty())
		{
			outData.clear();
			co_return;
		}

		auto file = co_await Windows::Storage::StorageFile::GetFileFromPathAsync(path);
		auto buffer = co_await Windows::Storage::FileIO::ReadBufferAsync(file);
		auto reader = Windows::Storage::Streams::DataReader::FromBuffer(buffer);

		std::vector<uint8_t> bytes(buffer.Length());
		reader.ReadBytes(bytes);

		outData = std::move(bytes);

		co_return;
	}
}