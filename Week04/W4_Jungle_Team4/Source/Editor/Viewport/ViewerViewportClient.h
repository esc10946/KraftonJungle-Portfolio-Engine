#pragma once
#include "Viewport/EditorViewportClient.h"

class UOrbitCameraComponent;
struct FStaticMeshAsset;

class FViewerViewportClient : public FEditorViewportClient
{
public:
	FViewerViewportClient() = default;
	~FViewerViewportClient() = default;

	// FEditorViewportClient 오버라이드
	/*
	* virtual void CreateCamera() override;
	virtual void ResetCamera()  override;
	virtual void TickInput(float DeltaTime) override;
	*/

	// 뷰어에서 표시할 정적 메시 asset을 로드합니다.
	void LoadStaticMeshAsset(const std::string& FilePath);

private:
	FStaticMeshAsset* CurrentMesh = nullptr;
};
