#pragma once
#include "RenderBus.h"
#include "Render/Resource/MeshBufferManager.h"

class UWorld;
class AActor;
class UPrimitiveComponent;
class UGizmoComponent;
class UStaticMeshComponent;
class UStaticMesh;

class FRenderCollector {
private:
	FMeshBufferManager MeshBufferManager;
	ID3D11Device* Device = nullptr;
	TMap<const UStaticMeshComponent*, FTextureMeshBuffer> StaticMeshBufferMap;
	TMap<const UStaticMeshComponent*, const UStaticMesh*> StaticMeshMap;
	TMap<const UStaticMeshComponent*, uint32> StaticMeshRevisionMap;
public:
	void Initialize(ID3D11Device* InDevice) { Device = InDevice; MeshBufferManager.Create(InDevice); }
	void Release();

	void CollectWorld(UWorld* World, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectSelection(const TArray<AActor*>& SelectedActors, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectGizmo(UGizmoComponent* Gizmo, const FShowFlags& ShowFlags, FRenderBus& RenderBus);
	void CollectGrid(float GridSpacing, int32 GridHalfLineCount, FRenderBus& RenderBus);

private:
	void CollectFromActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectFromSelectedActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectFromComponent(UPrimitiveComponent* Primitive, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectAABBCommand(UPrimitiveComponent* PrimitiveComponent, const FShowFlags& ShowFlags, FRenderBus& RenderBus);
	FTextureMeshBuffer* GetOrCreateStaticMeshBuffer(UStaticMeshComponent* StaticMeshComponent);
};
