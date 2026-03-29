#include "MeshBufferManager.h"

void FMeshBufferManager::Create(ID3D11Device* InDevice)
{
	MeshBufferMap[EPrimitiveType::EPT_Cube].Create(InDevice, FMeshManager::GetCube());
	MeshBufferMap[EPrimitiveType::EPT_Sphere].Create(InDevice, FMeshManager::GetSphere());
	MeshBufferMap[EPrimitiveType::EPT_Plane].Create(InDevice, FMeshManager::GetPlane());
	MeshBufferMap[EPrimitiveType::EPT_TransGizmo].Create(InDevice, FMeshManager::GetTranslationGizmo());
	MeshBufferMap[EPrimitiveType::EPT_RotGizmo].Create(InDevice, FMeshManager::GetRotationGizmo());
	MeshBufferMap[EPrimitiveType::EPT_ScaleGizmo].Create(InDevice, FMeshManager::GetScaleGizmo());
	MeshBufferMap[EPrimitiveType::EPT_SubUV].Create(InDevice, FMeshManager::GetQuad());
	MeshBufferMap[EPrimitiveType::EPT_Text].Create(InDevice, FMeshManager::GetQuad());
}

void FMeshBufferManager::Release()
{
	for (auto& pair : MeshBufferMap)
	{
		pair.second.Release();
	}
	MeshBufferMap.clear();
}

FMeshBuffer& FMeshBufferManager::GetMeshBuffer(EPrimitiveType InPrimitiveType)
{
	auto it = MeshBufferMap.find(InPrimitiveType);
	if (it != MeshBufferMap.end())
	{
		return it->second;
	}

	return MeshBufferMap.at(EPrimitiveType::EPT_Cube);
}
