#pragma once

#include "Object/ObjectFactory.h"

struct ID3D11Device;
struct ID3D11ShaderResourceView;

class UTexture2D : public UObject
{
public:
	DECLARE_CLASS(UTexture2D, UObject)

	UTexture2D() = default;
	~UTexture2D() override = default;

	void SetSourcePath(const FString& InSourcePath);
	const FString& GetSourcePath() const { return SourcePath; }
	ID3D11ShaderResourceView* GetShaderResourceView() const { return ShaderResourceView; }
	ID3D11ShaderResourceView* GetOrCreateShaderResourceView(ID3D11Device* InDevice);
	void InvalidateShaderResourceView();
	FString GetDisplayName() const;

private:
	FString SourcePath;
	ID3D11ShaderResourceView* ShaderResourceView = nullptr;
};
