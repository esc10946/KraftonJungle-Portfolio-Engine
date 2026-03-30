#include "Asset/Texture/TextureManager.h"

#include "Asset/Texture/TextureObject.h"
#include "Object/FName.h"
#include "Object/Object.h"

#include <filesystem>

UTexture2D* FTextureManager::GetOrCreateTexture(const FString& InTexturePath)
{
	if (InTexturePath.empty())
	{
		return nullptr;
	}

	if (UTexture2D* FoundTexture = FindTexture(InTexturePath))
	{
		return FoundTexture;
	}

	UTexture2D* NewTexture = ConstructObject<UTexture2D>();
	NewTexture->SetSourcePath(InTexturePath);
	NewTexture->SetFName(FName(std::filesystem::path(InTexturePath).filename().string()));

	TextureMap[InTexturePath] = NewTexture;
	return NewTexture;
}

UTexture2D* FTextureManager::FindTexture(const FString& InTexturePath) const
{
	const auto It = TextureMap.find(InTexturePath);
	return (It != TextureMap.end()) ? It->second : nullptr;
}

void FTextureManager::InvalidateAllTextureViews()
{
	for (auto& Pair : TextureMap)
	{
		if (Pair.second)
		{
			Pair.second->InvalidateShaderResourceView();
		}
	}
}

void FTextureManager::Clear()
{
	TextureMap.clear();
}
