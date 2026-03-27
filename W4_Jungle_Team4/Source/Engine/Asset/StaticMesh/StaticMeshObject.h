#pragma once

#include "Object/ObjectFactory.h"
#include "Render/Resource/Buffer.h"
#include <memory>

struct FStaticMeshAsset;

class UStaticMesh : public UObject
{
public:
	DECLARE_CLASS(UStaticMesh, UObject)

	UStaticMesh() = default;
	~UStaticMesh() override;

	void SetStaticMeshAsset(FStaticMeshAsset* InStaticMeshAsset);

	FStaticMeshAsset* GetStaticMeshAsset() const { return StaticMeshAsset; }
	const FString& GetAssetPathFileName() const;
	FString GetDisplayName() const;
	bool HasStaticMeshAsset() const { return StaticMeshAsset != nullptr; }

    FMeshBuffer* GetMeshBuffer() const { return MeshBuffer.get(); }
    FMeshBuffer* GetOrCreateMeshBuffer(ID3D11Device* Device);
    void ReleaseMeshBuffer();

private:
	FStaticMeshAsset* StaticMeshAsset = nullptr;
    std::unique_ptr<FMeshBuffer> MeshBuffer;
    FMeshData RenderMeshData;
};
