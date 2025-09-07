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
	// Prints message in the terminal
	inline void DebugOutput(const std::string& info)
	{
#ifdef _DEBUG
		//printf("\n%s", info.c_str());
		std::wcout << info.c_str() << std::endl;
#endif
	}

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

	// Loads packge file (for example .cso as bytes array
	inline std::vector<uint8_t> LoadPackageFile(const std::wstring& path) 
	{
		std::vector<uint8_t> fileData;
		auto tmpPath = Windows::ApplicationModel::Package::Current->InstalledLocation->Path;
		std::wstring installedPath(tmpPath->Data(), tmpPath->Length());
		std::wstring fullPath = installedPath + L"\\" + path;

		auto file = CreateFile2(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);

		if (file != INVALID_HANDLE_VALUE) 
		{
			DWORD readed;
			LARGE_INTEGER pos;
			LARGE_INTEGER newPos;
			LARGE_INTEGER fileSize;

			pos.QuadPart = 0;
			newPos.QuadPart = 0;

			SetFilePointerEx(file, pos, &newPos, FILE_END);
			fileSize = newPos;
			SetFilePointerEx(file, pos, &newPos, FILE_BEGIN);

			fileData.resize(static_cast<size_t>(fileSize.QuadPart));

			if (!ReadFile(file, fileData.data(), fileData.size(), &readed, nullptr))
			{
				CloseHandle(file);

				DWORD dwError = GetLastError();
				ThrowIfFailed(HRESULT_FROM_WIN32(dwError), "Error reading file!");

				return fileData;
			}

			CloseHandle(file);
		}

		return fileData;
	}
}
