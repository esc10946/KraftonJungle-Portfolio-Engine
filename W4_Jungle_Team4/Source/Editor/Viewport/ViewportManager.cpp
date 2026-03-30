#include "ViewportManager.h"
#include "Viewport/Viewport.h"
#include "Viewport/EditorViewportClient.h"
#include "Selection/SelectionManager.h"
#include "Settings/EditorSettings.h"
#include "Render/Device/D3DDevice.h"
#include "Slate/SlateApplication.h"
#include "Slate/SlateWidgetFactory.h"
#include "Slate/SViewport.h"
#include "GameFramework/World.h"

void FViewportManager::Initialize(FD3DDevice* InDevice, UWorld* InWorld, FSelectionManager* InSelectionManager)
{
	Device = InDevice;
	World = InWorld;
	SelectionManager = InSelectionManager;
}

void FViewportManager::SetWorld(UWorld* InWorld)
{
	World = InWorld;

	for (FViewport* Viewport : Viewports)
	{
		if (!Viewport)
		{
			continue;
		}

		FEditorViewportClient* EditorClient = dynamic_cast<FEditorViewportClient*>(Viewport->GetViewportClient());
		if (!EditorClient)
		{
			continue;
		}

		EditorClient->SetWorld(World);

		if (World)
		{
			World->SetActiveCamera(EditorClient->GetCamera());
		}
	}
}

FViewport* FViewportManager::CreateViewport()
{
	if (Device == nullptr) return nullptr;

	FViewport* NewViewport = new FViewport(Device);
	Viewports.push_back(NewViewport);
	return NewViewport;
}

void FViewportManager::DestroyViewport(FViewport* InViewport)
{
	auto it = std::find(Viewports.begin(), Viewports.end(), InViewport);
	if (it != Viewports.end())
	{
		delete* it;
		Viewports.erase(it);
	}
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
