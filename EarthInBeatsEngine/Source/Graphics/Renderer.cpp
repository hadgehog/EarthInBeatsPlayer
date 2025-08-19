#include "Renderer.h"
#include "Graphics.Renderer.g.cpp"

namespace winrt::EarthInBeatsEngine::Graphics::implementation
{
	Renderer::Renderer()
	{

	}

	//void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Initialize(
	//	const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel,
	//	const Windows::Foundation::Collections::IVector<IModel>& models,
	//	const Windows::Foundation::Collections::IVector<hstring>& bgTextures,
	//	int32_t width, int32_t height)
	//{
	//	auto model = models.First();
	//	auto modelPath = model->GetModelPath();
	//
	//	m_renderer = std::make_unique<NativeRenderer>();
	//	m_renderer->Initialize(reinterpret_cast<IUnknown*>(get_abi(panel)), width, height, modelPath);
	//}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Initialize(
		const winrt::Windows::UI::Xaml::Controls::SwapChainPanel& panel,
		uint32_t width, uint32_t height,
		const hstring& modelPath,
		const hstring& backgroundTexturePath,
		const hstring& modelTextureOverridePath)
	{
		m_renderer = std::make_unique<NativeRenderer>();
		m_renderer->Initialize(reinterpret_cast<IUnknown*>(get_abi(panel)), width, height, winrt::to_string(modelPath));
	}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Resize(uint32_t width, uint32_t height)
	{
		m_renderer->Resize(width, height);
	}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Render()
	{
		m_renderer->Draw();
	}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Shutdown()
	{
		// TODO: implement
	}

	//void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::LoadModels(const Windows::Foundation::Collections::IVector<IModel>& models)
	//{
	//	// TODO: implement
	//}
	//
	//void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::LoadBackgrounds(const Windows::Foundation::Collections::IVector<hstring>& textures)
	//{
	//	// TODO: implement
	//}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::SetRotation(float yaw, float pitch)
	{
		// TODO: implement
	}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::SetZoom(float delta)
	{
		// TODO: implement
	}

	bool winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Manipulation() const
	{
		// TODO: implement
		return false;
	}

	void winrt::EarthInBeatsEngine::Graphics::implementation::Renderer::Manipulation(bool isManipulating)
	{
		// TODO: implement
	}
}