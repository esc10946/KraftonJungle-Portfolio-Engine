#pragma once

#include "Core/CoreTypes.h"
#include "Core/Singleton.h"
#include "Asset/Shader/ShaderObject.h"

class FShaderManager : public TSingleton<FShaderManager>
{
	friend class TSingleton<FShaderManager>;

public:
	UShader* FindShader(const FString& InShaderKey) const;
	UShader* GetOrCreateShader(
		const FString& InShaderKey,
		EShaderDomain InDomain,
		const FString& InSourcePath,
		const FString& InVertexEntryPoint = "VS",
		const FString& InPixelEntryPoint = "PS");
	UShader* GetOrCreateStaticMeshShader();
	void Clear();

private:
	FShaderManager() = default;

private:
	TMap<FString, UShader*> ShaderMap;
};
