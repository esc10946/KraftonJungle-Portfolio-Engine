#include "Asset/Shader/ShaderObject.h"

#include <filesystem>

DEFINE_CLASS(UShader, UObject)
REGISTER_FACTORY(UShader)

FString UShader::GetDisplayName() const
{
	if (!ShaderKey.empty())
	{
		return ShaderKey;
	}

	if (!SourcePath.empty())
	{
		return std::filesystem::path(SourcePath).filename().string();
	}

	return GetFName().ToString();
}
