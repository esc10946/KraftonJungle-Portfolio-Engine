#include "ViewportManager.h"
#include "Viewport/Viewport.h"
#include "Viewport/EditorViewportClient.h"
#include "Selection/SelectionManager.h"
#include "Settings/EditorSettings.h"
#include "Render/Device/D3DDevice.h"

void FViewportManager::Initialize(FD3DDevice* InDevice, UWorld* InWorld, FSelectionManager* InSelectionManager)
{
	Device = InDevice;
	World = InWorld;
	SelectionManager = InSelectionManager;
}

FViewport* FViewportManager::CreateViewport()
{
	if (Device == nullptr) return nullptr;

	FViewport* NewViewport = new FViewport(Device);
	Viewports.push_back(NewViewport);
	return NewViewport;
}

FEditorViewportClient* FViewportManager::CreateEditorViewportClient() const
{
	FEditorViewportClient* NewClient = new FEditorViewportClient();
	NewClient->SetSettings(&FEditorSettings::Get());
	NewClient->SetWorld(World);
	NewClient->SetGizmo(SelectionManager->GetGizmo());
	NewClient->SetSelectionManager(SelectionManager);
	NewClient->CreateCamera();
	NewClient->ResetCamera();

	return NewClient;
}

void FViewportManager::DestroyViewport(FViewport* InViewport)
{
	auto it = std::find(Viewports.begin(), Viewports.end(), InViewport);
	if (it != Viewports.end())
	{
		delete *it;
		Viewports.erase(it);
	}
}
