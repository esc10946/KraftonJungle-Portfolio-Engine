#pragma once
#include "Object/Object.h"
#include "StaticMeshAsset.h"

class UMaterial;

class UStaticMesh :
    public UObject
{
public:
    UStaticMesh()           = default;
    ~UStaticMesh() override;
	
	void Release();

	DECLARE_CLASS(UStaticMesh, UObject)
	//TODO: 나중에 MeshAsset에 IsVaild함수로 해당 변수를 사용가능한지 결정

    // CPU 데이터 (FObjImporter가 채움)
    FStaticMeshAsset& GetMeshAsset()             { return MeshAsset; }
    const FStaticMeshAsset& GetMeshAsset() const { return MeshAsset; }
    void SetMeshAsset(FStaticMeshAsset& inMeshAsset) { MeshAsset = inMeshAsset; }
	
	const FString& GetSourcePath() const         { return MeshAsset.SourcePath; }
    bool           IsValid()       const         { return !MeshAsset.Vertices.empty() && !MeshAsset.Indices.empty(); }
	//TODO: MeshAsset에 IsValid함수로 값이있는지 여부

    // GPU 핸들 (렌더러가 꽂아줌, 에셋은 DX 모름)
    void  SetGPUBuffers(void* VB, void* IB) { VertexBuffer = VB; IndexBuffer = IB; }
    void* GetVertexBuffer() const { return VertexBuffer; }
    void* GetIndexBuffer()  const { return IndexBuffer; }
    bool  IsGPUReady()      const { return VertexBuffer && IndexBuffer; }

	void  MarkGPUDirty()        { bGPUDirty = true;  }
    bool  IsGPUDirty()    const { return bGPUDirty;  }
    void  ClearGPUDirty()       { bGPUDirty = false; }

    // 머티리얼 슬롯 (섹션 MaterialName → UMaterial 참조)
    void       SetMaterial(const FString& SlotName, UMaterial* InMaterial);
    UMaterial* GetMaterial(const FString& SlotName) const;
	
    int32      GetNumMaterials() const { return static_cast<int32>(MaterialSlots.size()); }

    const TArray<UMaterial*>& GetMaterialSlots() const { return MaterialSlots; }
private:
    FStaticMeshAsset MeshAsset;    // CPU 데이터

    // 머티리얼 슬롯 — FStaticMeshSection::MaterialIndex 로 접근
    TArray<UMaterial*>  MaterialSlots;
	
	//생성할때 렌더러가 처음 그릴떄 주입함
    void* VertexBuffer = nullptr;  // GPU 핸들
    void* IndexBuffer  = nullptr;  // GPU 핸들
	bool  bGPUDirty    = true;   // ← 처음엔 무조건 dirty
};

