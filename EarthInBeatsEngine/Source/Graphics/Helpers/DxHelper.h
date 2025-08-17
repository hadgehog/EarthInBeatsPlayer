#pragma once

#include <ppltasks.h>	// For create_task
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <string>
#include <iostream>

#define G_MAX_SWAPCHAIN_BUFFERS 2

#define PRINT_MSG(message) std::cout << message << std::endl;

namespace DX
{
	// Function for validation HRESULT resuts.
	inline void ThrowIfFailed(HRESULT hr, Platform::String^ msg = "")
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw Platform::Exception::CreateException(hr, msg);
		}
	}

	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([] (StorageFile^ file) 
		{
			return FileIO::ReadBufferAsync(file);
		}).then([] (Streams::IBuffer^ fileBuffer) -> std::vector<byte> 
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

	// Converts std::wstring to std::string
	inline std::string WideCharToUtf8(const std::wstring& wStr)
	{
		if (wStr.empty())
			return std::string();

		int size = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), (int)wStr.size(), nullptr, 0, nullptr, nullptr);
		std::string str(size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), (int)wStr.size(), str.data(), size, nullptr, nullptr);

		return str;
	}
}
