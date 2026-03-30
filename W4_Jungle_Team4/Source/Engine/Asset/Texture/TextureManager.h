#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"

class UTexture2D;

class FTextureManager : public TSingleton<FTextureManager>
{
	friend class TSingleton<FTextureManager>;

public:
	UTexture2D* GetOrCreateTexture(const FString& InTexturePath);
	UTexture2D* FindTexture(const FString& InTexturePath) const;
	void InvalidateAllTextureViews();
	void Clear();

private:
	FTextureManager() = default;

private:
	TMap<FString, UTexture2D*> TextureMap;
};
