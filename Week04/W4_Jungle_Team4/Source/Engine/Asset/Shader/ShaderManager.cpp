#include "Asset/Shader/ShaderManager.h"

#include "Object/FName.h"
#include "Object/Object.h"

FShaderManager::~FShaderManager()
{
	Clear();
}

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
	for (auto& Pair : ShaderMap)
	{
		delete Pair.second; // 임의로 매니저에서 UShader를 삭제하도록 관리
	}

	ShaderMap.clear();
}
