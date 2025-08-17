#pragma once

#include "Graphics.Renderer.g.h"

#include "Renderer/NativeRenderer.h"

namespace winrt::EarthInBeatsEngine::Graphics::implementation
{
	struct Renderer : RendererT<Renderer>
	{
	public:
		Renderer();
		~Renderer();

		void InitRenderer(const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel, 
			const Windows::Foundation::Collections::IVector<IModel>& models, 
			const Windows::Foundation::Collections::IVector<hstring>& textures);

		void LoadModels(const Windows::Foundation::Collections::IVector<IModel>& models);
		void LoadBackgrounds(const Windows::Foundation::Collections::IVector<hstring>& textures);

		void StartRendering();
		void StopRendering();

		void SetRotation(float yaw, float pitch);
		void SetZoom(float delta);

		bool Manipulation() const;
		void Manipulation(bool isManipulating);

	private:
		std::unique_ptr<NativeRenderer> renderer { nullptr };
	};
}

namespace winrt::EarthInBeatsEngine::Graphics::factory_implementation
{
	struct Renderer : RendererT<Renderer, implementation::Renderer>
	{
	};
}
