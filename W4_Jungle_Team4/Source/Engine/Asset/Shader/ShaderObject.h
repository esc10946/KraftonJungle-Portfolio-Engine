#pragma once

#include "Object/ObjectFactory.h"

enum class EShaderDomain : uint8
{
	Unknown = 0,
	Primitive,
	StaticMesh,
	Gizmo,
	Editor,
	Outline,
};

class UShader : public UObject
{
public:
	DECLARE_CLASS(UShader, UObject)

	UShader() = default;
	~UShader() override = default;

	void SetShaderKey(const FString& InShaderKey) { ShaderKey = InShaderKey; }
	const FString& GetShaderKey() const { return ShaderKey; }

	void SetSourcePath(const FString& InSourcePath) { SourcePath = InSourcePath; }
	const FString& GetSourcePath() const { return SourcePath; }

	void SetVertexEntryPoint(const FString& InEntryPoint) { VertexEntryPoint = InEntryPoint; }
	const FString& GetVertexEntryPoint() const { return VertexEntryPoint; }

	void SetPixelEntryPoint(const FString& InEntryPoint) { PixelEntryPoint = InEntryPoint; }
	const FString& GetPixelEntryPoint() const { return PixelEntryPoint; }

	void SetDomain(EShaderDomain InDomain) { Domain = InDomain; }
	EShaderDomain GetDomain() const { return Domain; }

	FString GetDisplayName() const;

private:
	FString ShaderKey;
	FString SourcePath;
	FString VertexEntryPoint = "VS";
	FString PixelEntryPoint = "PS";
	EShaderDomain Domain = EShaderDomain::Unknown;
};
