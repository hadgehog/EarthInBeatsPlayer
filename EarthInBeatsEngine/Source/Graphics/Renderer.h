#pragma once

#include "Graphics.Renderer.g.h"
#include <winrt/Windows.UI.Xaml.Controls.h>

class NativeRenderer;

namespace winrt::EarthInBeatsEngine::Graphics::implementation
{
	struct Renderer : RendererT<Renderer>
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		// TODO : implement
		//void Initialize(
		//  const Windows::UI::Xaml::Controls::SwapChainPanel& panel,
		//	const Windows::Foundation::Collections::IVector<IModel>& models, 
		//	const Windows::Foundation::Collections::IVector<hstring>& bgTextures,
		//	int32_t width, int32_t height);

		Windows::Foundation::IAsyncAction Initialize(
			const Windows::UI::Xaml::Controls::SwapChainPanel& panel,
			uint32_t width, 
			uint32_t height,
			const hstring& modelPath,
			const hstring& backgroundTexturePath,
			const hstring& modelTextureOverridePath);

		void Resize(uint32_t width, uint32_t height);

		void Render();
		void Shutdown();

		// TODO: implement
		//void LoadModels(const Windows::Foundation::Collections::IVector<IModel>& models);
		//void LoadBackgrounds(const Windows::Foundation::Collections::IVector<hstring>& textures);

		// TODO: implement
		void SetRotation(float yaw, float pitch);
		void SetZoom(float delta);

		// TODO: implement
		bool Manipulation() const;
		void Manipulation(bool isManipulating);

	private:
		// TODO: return data in other way than out param
		Windows::Foundation::IAsyncAction LoadAssetFromAppxPath(const hstring& path, std::vector<uint8_t>& outData);

	private:
		std::unique_ptr<NativeRenderer> m_renderer { nullptr };
	};
}

namespace winrt::EarthInBeatsEngine::Graphics::factory_implementation
{
	struct Renderer : RendererT<Renderer, implementation::Renderer>
	{
	};
}
