#pragma once

#include "Graphics.Renderer.g.h"

namespace winrt::EarthInBeatsEngine::Graphics::implementation
{
	struct Renderer : RendererT<Renderer>
	{
	public:
		Renderer();
		~Renderer();

		void InitRenderer(const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel);

		void LoadModel(const hstring& path);
		void LoadModelTexture(const hstring& path);
		void LoadBackgroundTexture(const hstring& path);

		void StartRendering();
		void StopRendering();

		void SetRotation(float yaw, float pitch);

	private:
	};
}

namespace winrt::EarthInBeatsEngine::Graphics::factory_implementation
{
	struct Renderer : RendererT<Renderer, implementation::Renderer>
	{
	};
}
