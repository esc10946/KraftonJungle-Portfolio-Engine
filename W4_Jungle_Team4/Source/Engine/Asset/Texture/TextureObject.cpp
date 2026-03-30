#include "Asset/Texture/TextureObject.h"

#include <filesystem>

DEFINE_CLASS(UTexture2D, UObject)
REGISTER_FACTORY(UTexture2D)

FString UTexture2D::GetDisplayName() const
{
	if (SourcePath.empty())
	{
		return GetFName().ToString();
	}

	return std::filesystem::path(SourcePath).filename().string();
}
