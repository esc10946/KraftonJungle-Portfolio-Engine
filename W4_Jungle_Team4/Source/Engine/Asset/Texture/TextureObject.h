#pragma once

#include "Object/ObjectFactory.h"

class UTexture2D : public UObject
{
public:
	DECLARE_CLASS(UTexture2D, UObject)

	UTexture2D() = default;
	~UTexture2D() override = default;

	void SetSourcePath(const FString& InSourcePath) { SourcePath = InSourcePath; }
	const FString& GetSourcePath() const { return SourcePath; }
	FString GetDisplayName() const;

private:
	FString SourcePath;
};
