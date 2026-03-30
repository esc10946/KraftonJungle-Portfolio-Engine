#include "Asset/Shader/ShaderManager.h"

#include "Object/FName.h"
#include "Object/Object.h"

UShader* FShaderManager::FindShader(const FString& InShaderKey) const
{
	const auto It = ShaderMap.find(InShaderKey);
	return (It != ShaderMap.end()) ? It->second : nullptr;
}

UShader* FShaderManager::GetOrCreateShader(
	const FString& InShaderKey,
	EShaderDomain InDomain,
	const FString& InSourcePath,
	const FString& InVertexEntryPoint,
	const FString& InPixelEntryPoint)
{
	if (InShaderKey.empty())
	{
		return nullptr;
	}

	if (UShader* FoundShader = FindShader(InShaderKey))
	{
		return FoundShader;
	}

	UShader* NewShader = ConstructObject<UShader>();
	NewShader->SetShaderKey(InShaderKey);
	NewShader->SetSourcePath(InSourcePath);
	NewShader->SetVertexEntryPoint(InVertexEntryPoint);
	NewShader->SetPixelEntryPoint(InPixelEntryPoint);
	NewShader->SetDomain(InDomain);
	NewShader->SetFName(FName(InShaderKey));
	ShaderMap[InShaderKey] = NewShader;
	return NewShader;
}

UShader* FShaderManager::GetOrCreateStaticMeshShader()
{
	return GetOrCreateShader(
		"StaticMeshTextured",
		EShaderDomain::StaticMesh,
		"Shaders/StaticMeshTextured.hlsl",
		"VS",
		"PS");
}

void FShaderManager::Clear()
{
	ShaderMap.clear();
}
