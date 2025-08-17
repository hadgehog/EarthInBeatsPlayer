#include "Renderer.h"
#include "Graphics.Renderer.g.cpp"

winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Renderer()
{
}

winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::~Renderer()
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::InitRenderer(
	const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel,
	const Windows::Foundation::Collections::IVector<IModel>& models,
	const Windows::Foundation::Collections::IVector<hstring>& textures)
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::LoadModels(const Windows::Foundation::Collections::IVector<IModel>& models)
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::LoadBackgrounds(const Windows::Foundation::Collections::IVector<hstring>& textures)
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::StartRendering()
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::StopRendering()
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::SetRotation(float yaw, float pitch)
{
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::SetZoom(float delta)
{

}

bool winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Manipulation() const
{
	return false;
}

void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Manipulation(bool isManipulating)
{

}