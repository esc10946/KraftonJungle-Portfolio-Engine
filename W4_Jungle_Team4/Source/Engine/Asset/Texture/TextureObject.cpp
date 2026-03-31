#include "Asset/Texture/TextureObject.h"

#include "Core/ResourceManager.h"

#include <filesystem>
#include <iostream>

DEFINE_CLASS(UTexture2D, UObject)
REGISTER_FACTORY(UTexture2D)

void UTexture2D::SetSourcePath(const FString& InSourcePath)
{
	SourcePath = InSourcePath;
	ShaderResourceView = nullptr;
}

ID3D11ShaderResourceView* UTexture2D::GetOrCreateShaderResourceView(ID3D11Device* InDevice)
{
	if (!ShaderResourceView && !SourcePath.empty())
	{
		FResourceManager::Get().RegisterTexture(SourcePath);
		ShaderResourceView = FResourceManager::Get().GetOrCreateTextureSRV(SourcePath, InDevice);
	}

	return ShaderResourceView;
}

void UTexture2D::InvalidateShaderResourceView()
{
	ShaderResourceView = nullptr;
}

FString UTexture2D::GetDisplayName() const
{
	if (SourcePath.empty())
	{
		return GetFName().ToString();
	}

	return std::filesystem::path(SourcePath).filename().string();
}
