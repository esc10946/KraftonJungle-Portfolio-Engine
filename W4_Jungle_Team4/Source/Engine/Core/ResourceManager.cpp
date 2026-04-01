#include "Core/ResourceManager.h"
#include "Asset/Texture/TextureManager.h"
#include "Core/Paths.h"
#include "SimpleJSON/json.hpp"
#include "Slate/ConsoleCommandManager.h"
#include "Object/EngineStatics.h"

#include <d3d11.h>
#include <fstream>
#include <filesystem>
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

using namespace std::filesystem;

namespace ResourceKey
{
	constexpr const char* Font     = "Font";
	constexpr const char* Particle = "Particle";
	constexpr const char* Path     = "Path";
	constexpr const char* Columns  = "Columns";
	constexpr const char* Rows     = "Rows";
	constexpr const char* StaticMesh = "StaticMesh";
}

namespace
{
	uint32 GetBitsPerPixel(DXGI_FORMAT Format)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return 32;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
			return 16;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
			return 8;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return 4;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;

		default:
			return 32; // Default to 32bpp
		}
	}

	void UpdateTextureResourceMemory(ID3D11ShaderResourceView* SRV, uint32& OutMemorySize)
	{
		if (!SRV) return;

		ID3D11Resource* Resource = nullptr;
		SRV->GetResource(&Resource);
		if (Resource)
		{
			ID3D11Texture2D* Texture2D = nullptr;
			if (SUCCEEDED(Resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&Texture2D)))
			{
				D3D11_TEXTURE2D_DESC Desc;
				Texture2D->GetDesc(&Desc);

				uint32 BitsPerPixel = GetBitsPerPixel(Desc.Format);
				
				bool IsBC = (Desc.Format >= DXGI_FORMAT_BC1_TYPELESS && Desc.Format <= DXGI_FORMAT_BC5_SNORM) ||
				            (Desc.Format >= DXGI_FORMAT_BC6H_TYPELESS && Desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB);

				uint32 TotalSize = 0;
				uint32 MipWidth = Desc.Width;
				uint32 MipHeight = Desc.Height;
				uint32 MipLevels = (Desc.MipLevels == 0) ? 1 : Desc.MipLevels;

				for (uint32 i = 0; i < MipLevels; ++i)
				{
					if (IsBC)
					{
						uint32 NumBlocksWide = (MipWidth + 3) / 4;
						uint32 NumBlocksHigh = (MipHeight + 3) / 4;
						uint32 BytesPerBlock = (Desc.Format == DXGI_FORMAT_BC1_UNORM || Desc.Format == DXGI_FORMAT_BC1_UNORM_SRGB || Desc.Format == DXGI_FORMAT_BC1_TYPELESS) ? 8 : 16;
						TotalSize += NumBlocksWide * NumBlocksHigh * BytesPerBlock;
					}
					else
					{
						TotalSize += (MipWidth * MipHeight * BitsPerPixel) / 8;
					}

					MipWidth = (MipWidth > 1) ? MipWidth >> 1 : 1;
					MipHeight = (MipHeight > 1) ? MipHeight >> 1 : 1;
				}

				OutMemorySize = TotalSize * Desc.ArraySize;

				Texture2D->Release();
			}
			Resource->Release();
		}
	}
}

void FResourceManager::LoadFromFile(const FString& Path, ID3D11Device* InDevice)
{
	using namespace json;

	std::ifstream File(std::filesystem::path(FPaths::ToWide(Path)));
	if (!File.is_open())
	{
		return;
	}

	FString Content((std::istreambuf_iterator<char>(File)),
		std::istreambuf_iterator<char>());

	JSON Root = JSON::Load(Content);

	// Font — { "Name": { "Path": "...", "Columns": 16, "Rows": 16 } }
	if (Root.hasKey(ResourceKey::Font))
	{
		JSON FontSection = Root[ResourceKey::Font];
		for (auto& Pair : FontSection.ObjectRange())
		{
			JSON Entry = Pair.second;
			FFontResource Resource;
			Resource.Name    = FName(Pair.first.c_str());
			Resource.Path    = Entry[ResourceKey::Path].ToString();
			Resource.Columns = static_cast<uint32>(Entry[ResourceKey::Columns].ToInt());
			Resource.Rows    = static_cast<uint32>(Entry[ResourceKey::Rows].ToInt());
			Resource.SRV     = nullptr;
			FontResources[Pair.first] = Resource;
		}
	}

	// Particle — { "Name": { "Path": "...", "Columns": 6, "Rows": 6 } }
	if (Root.hasKey(ResourceKey::Particle))
	{
		JSON ParticleSection = Root[ResourceKey::Particle];
		for (auto& Pair : ParticleSection.ObjectRange())
		{
			JSON Entry = Pair.second;
			FParticleResource Resource;
			Resource.Name    = FName(Pair.first.c_str());
			Resource.Path    = Entry[ResourceKey::Path].ToString();
			Resource.Columns = static_cast<uint32>(Entry[ResourceKey::Columns].ToInt());
			Resource.Rows    = static_cast<uint32>(Entry[ResourceKey::Rows].ToInt());
			Resource.SRV     = nullptr;
			ParticleResources[Pair.first] = Resource;
		}
	}

	if (Root.hasKey(ResourceKey::StaticMesh))
	{
		JSON StaticMeshSection = Root[ResourceKey::StaticMesh];
		for (auto& Pair : StaticMeshSection.ObjectRange())
		{
			JSON Entry = Pair.second;
			StaticMeshPaths.push_back(Entry[ResourceKey::Path].ToString());
		}
	}

	if (LoadGPUResources(InDevice))
	{
		UE_LOG("Complete Load Resources!");
	}
	else
	{
		UE_LOG("Failed to Load Resources...");
	}
}

bool FResourceManager::LoadGPUResources(ID3D11Device* Device)
{
	if (!Device)
	{
		return false;
	}

	auto LoadSRV = [&](FTextureAtlasResource& Resource) -> bool
	{
		std::wstring FullPath = FPaths::Combine(FPaths::RootDir(), FPaths::ToWide(Resource.Path));
		HRESULT hr = DirectX::CreateDDSTextureFromFileEx(
			Device,
			FullPath.c_str(),
			0,
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_SHADER_RESOURCE,
			0, 0,
			DirectX::DDS_LOADER_DEFAULT,
			nullptr,
			&Resource.SRV
		);

		if (SUCCEEDED(hr))
		{
			UpdateTextureResourceMemory(Resource.SRV, Resource.MemorySize);
			EngineStatics::AddMemoryUsage(EMemoryCategory::Texture, Resource.MemorySize);
		}

		return SUCCEEDED(hr);
	};

	for (auto& [Key, Resource] : FontResources)
	{
		if (!LoadSRV(Resource)) return false;
	}

	for (auto& [Key, Resource] : ParticleResources)
	{
		if (!LoadSRV(Resource)) return false;
	}

	return true;
}

void FResourceManager::RegisterTexture(const FString& InTexturePath)
{
	if (InTexturePath.empty())
	{
		return;
	}

	if (TextureResources.find(InTexturePath) != TextureResources.end())
	{
		return;
	}

	FTextureResource Resource;
	Resource.Path = InTexturePath;
	Resource.SRV = nullptr;
	TextureResources[InTexturePath] = Resource;
}

FTextureResource* FResourceManager::FindTexture(const FString& InTexturePath)
{
	auto It = TextureResources.find(InTexturePath);
	return (It != TextureResources.end()) ? &It->second : nullptr;
}

const FTextureResource* FResourceManager::FindTexture(const FString& InTexturePath) const
{
	auto It = TextureResources.find(InTexturePath);
	return (It != TextureResources.end()) ? &It->second : nullptr;
}

bool FResourceManager::LoadTextureResource(FTextureResource& Resource, ID3D11Device* InDevice)
{
	if (!InDevice || Resource.Path.empty())
	{
		return false;
	}

	if (Resource.IsLoaded())
	{
		return true;
	}

	std::filesystem::path TexturePath(Resource.Path);
	if (!TexturePath.is_absolute())
	{
		TexturePath = std::filesystem::path(FPaths::RootDir()) / TexturePath;
	}

	TexturePath = TexturePath.lexically_normal();

	HRESULT Hr = E_FAIL;
	if (TexturePath.extension() == ".dds")
	{
		Hr = DirectX::CreateDDSTextureFromFileEx(
			InDevice,
			TexturePath.wstring().c_str(),
			0,
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_SHADER_RESOURCE,
			0, 0,
			DirectX::DDS_LOADER_DEFAULT,
			nullptr,
			&Resource.SRV
		);
	}
	else
	{
		Hr = DirectX::CreateWICTextureFromFile(
			InDevice,
			TexturePath.wstring().c_str(),
			nullptr,
			&Resource.SRV
		);
	}

	if (SUCCEEDED(Hr))
	{
		UpdateTextureResourceMemory(Resource.SRV, Resource.MemorySize);
		EngineStatics::AddMemoryUsage(EMemoryCategory::Texture, Resource.MemorySize);
	}

	return SUCCEEDED(Hr);
}

ID3D11ShaderResourceView* FResourceManager::GetOrCreateTextureSRV(const FString& InTexturePath, ID3D11Device* InDevice)
{
	std::filesystem::path P = InTexturePath;
	if (InTexturePath.empty() || !InDevice)
	{
		return nullptr;
	}

	RegisterTexture(InTexturePath);

	FTextureResource* TextureResource = FindTexture(InTexturePath);
	if (!TextureResource)
	{
		return nullptr;
	}

	if (!TextureResource->IsLoaded())
	{
		if (!LoadTextureResource(*TextureResource, InDevice))
		{
			return nullptr;
		}
	}

	return TextureResource->SRV;
}

void FResourceManager::ReleaseGPUResources()
{
	for (auto& [Key, Resource] : FontResources)
	{
		if (Resource.SRV) 
		{ 
			EngineStatics::SubtractMemoryUsage(EMemoryCategory::Texture, Resource.MemorySize);
			Resource.SRV->Release(); 
			Resource.SRV = nullptr; 
			Resource.MemorySize = 0;
		}
	}
	for (auto& [Key, Resource] : ParticleResources)
	{
		if (Resource.SRV) 
		{ 
			EngineStatics::SubtractMemoryUsage(EMemoryCategory::Texture, Resource.MemorySize);
			Resource.SRV->Release(); 
			Resource.SRV = nullptr; 
			Resource.MemorySize = 0;
		}
	}

	ReleaseTextureResources();
}

void FResourceManager::ReleaseTextureResources()
{
	for (auto& [Key, Resource] : TextureResources)
	{
		if (Resource.SRV)
		{
			EngineStatics::SubtractMemoryUsage(EMemoryCategory::Texture, Resource.MemorySize);
			Resource.SRV->Release();
			Resource.SRV = nullptr;
			Resource.MemorySize = 0;
		}
	}

	FTextureManager::Get().InvalidateAllTextureViews();
}

// --- Font ---
FFontResource* FResourceManager::FindFont(const FName& FontName)
{
	auto It = FontResources.find(FontName.ToString());
	return (It != FontResources.end()) ? &It->second : nullptr;
}

const FFontResource* FResourceManager::FindFont(const FName& FontName) const
{
	auto It = FontResources.find(FontName.ToString());
	return (It != FontResources.end()) ? &It->second : nullptr;
}

void FResourceManager::RegisterFont(const FName& FontName, const FString& InPath, uint32 Columns, uint32 Rows)
{
	FFontResource Resource;
	Resource.Name    = FontName;
	Resource.Path    = InPath;
	Resource.Columns = Columns;
	Resource.Rows    = Rows;
	Resource.SRV     = nullptr;
	FontResources[FontName.ToString()] = Resource;
}

// --- Particle ---
FParticleResource* FResourceManager::FindParticle(const FName& ParticleName)
{
	auto It = ParticleResources.find(ParticleName.ToString());
	return (It != ParticleResources.end()) ? &It->second : nullptr;
}

const FParticleResource* FResourceManager::FindParticle(const FName& ParticleName) const
{
	auto It = ParticleResources.find(ParticleName.ToString());
	return (It != ParticleResources.end()) ? &It->second : nullptr;
}

void FResourceManager::RegisterParticle(const FName& ParticleName, const FString& InPath, uint32 Columns, uint32 Rows)
{
	FParticleResource Resource;
	Resource.Name    = ParticleName;
	Resource.Path    = InPath;
	Resource.Columns = Columns;
	Resource.Rows    = Rows;
	Resource.SRV     = nullptr;
	ParticleResources[ParticleName.ToString()] = Resource;
}

TArray<FString> FResourceManager::GetFontNames() const
{
	TArray<FString> Names;
	Names.reserve(FontResources.size());
	for (const auto& [Key, _] : FontResources)
	{
		Names.push_back(Key);
	}
	return Names;
}

TArray<FString> FResourceManager::GetParticleNames() const
{
	TArray<FString> Names;
	Names.reserve(ParticleResources.size());
	for (const auto& [Key, _] : ParticleResources)
	{
		Names.push_back(Key);
	}
	return Names;
}
