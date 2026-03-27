#pragma once
#include "Viewport/EditorViewportClient.h"

class UOrbitCameraComponent;
class FStaticMesh;

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

	// OBJ 로드
	void LoadOBJ(const std::string& FilePath);

private:
	FStaticMesh* CurrentMesh = nullptr;
};