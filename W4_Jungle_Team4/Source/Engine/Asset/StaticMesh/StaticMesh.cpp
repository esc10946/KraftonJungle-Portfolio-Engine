#include "StaticMesh.h"
#include "Render/Resource/Material.h"

DEFINE_CLASS(UStaticMesh, UObject)

UStaticMesh::~UStaticMesh()
{
	Release();
}

void UStaticMesh::Release()
{
	MeshAsset.Vertices.clear();
	MeshAsset.Indices.clear();
	MeshAsset.Sections.clear();

	//vb, ib 초기화

	for (auto& material : MaterialSlots) {
		material = nullptr;
	}
	MaterialSlots.clear();

	VertexBuffer = nullptr;  // GPU 핸들
    IndexBuffer  = nullptr;  // GPU 핸들
}