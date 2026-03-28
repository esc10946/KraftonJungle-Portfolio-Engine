#include "Editor/Viewport/EditorViewportClient.h"

#include "Editor/UI/EditorConsoleWidget.h"
#include "Editor/Settings/EditorSettings.h"
#include "Engine/Core/InputSystem.h"
#include "Engine/Runtime/WindowsWindow.h"

#include "Component/CameraComponent.h"
#include "Editor/Viewport/Viewport.h"
#include "GameFramework/World.h"
#include "Component/GizmoComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Object/Object.h"
#include "Editor/Selection/SelectionManager.h"

void FEditorViewportClient::Initialize(FViewport* InViewport)
{
	UE_LOG("Hello ZZup Engine! %d", 2026);

	Viewport = InViewport;
}

void FEditorViewportClient::CreateCamera()
{
	DestroyCamera();
	Camera = UObjectManager::Get().CreateObject<UCameraComponent>();
}

void FEditorViewportClient::DestroyCamera()
{
	if (Camera)
	{
		UObjectManager::Get().DestroyObject(Camera);
		Camera = nullptr;
	}
}

void FEditorViewportClient::ResetCamera()
{
	if (!Camera || !Settings) return;
	Camera->SetWorldLocation(Settings->InitViewPos);
	Camera->LookAt(Settings->InitLookAt);
}

void FEditorViewportClient::Tick(float DeltaTime)
{
	Move(DeltaTime);
	Rotate(DeltaTime);
	Zoom(DeltaTime);
}

void FEditorViewportClient::OnViewportResized(uint32 NewTopLeftX, uint32 NewTopLeftY, uint32 InWidth, uint32 InHeight)
{
	Viewport->Resize(NewTopLeftX, NewTopLeftY, InWidth, InHeight);
	
	if (Camera)
	{
		Camera->OnResize(InWidth, InHeight);
	}
}

void FEditorViewportClient::OnMouseDown(float X, float Y, int32 Button)
{
	if (Button == VK_LBUTTON)
	{
		bIsLeftMouseDown = true;
	}
	if (Button == VK_RBUTTON)
	{
		bIsRightMouseDown = true;
	}
}

void FEditorViewportClient::OnMouseUp(float X, float Y, int32 Button)
{
	if (Button == VK_LBUTTON)
	{
		bIsLeftMouseDown = false;
	}
	if (Button == VK_RBUTTON)
	{
		bIsRightMouseDown = false;
	}
}

void FEditorViewportClient::OnMouseMove(float X, float Y)
{
}

void FEditorViewportClient::OnMouseWheel(float Delta)
{
	MouseWheelDelta = Delta;
}

void FEditorViewportClient::OnKeyDown(int32 Key)
{
	if (Key == 'W') bIsWDown = true;
	if (Key == 'A') bIsADown = true;
	if (Key == 'S') bIsSDown = true;
	if (Key == 'D') bIsDDown = true;
	if (Key == 'Q') bIsQDown = true;
	if (Key == 'E') bIsEDown = true;

	const FCameraState& CameraState = Camera->GetCameraState();
	if (Key == 'O') Camera->SetOrthographic(!CameraState.bIsOrthogonal);
}

void FEditorViewportClient::OnKeyUp(int32 Key)
{
	if (Key == 'W') bIsWDown = false;
	if (Key == 'A') bIsADown = false;
	if (Key == 'S') bIsSDown = false;
	if (Key == 'D') bIsDDown = false;
	if (Key == 'Q') bIsQDown = false;
	if (Key == 'E') bIsEDown = false;
}

void FEditorViewportClient::Move(float DeltaTime)
{
	const FCameraState& CameraState = Camera->GetCameraState();

	FVector Direction = FVector(0, 0, 0);

	if (bIsWDown && !CameraState.bIsOrthogonal) Direction.X += 1;
	if (bIsADown) Direction.Y -= 1;
	if (bIsSDown && !CameraState.bIsOrthogonal) Direction.X -= 1;
	if (bIsDDown) Direction.Y += 1;
	if (bIsQDown) Direction.Z -= 1;
	if (bIsEDown) Direction.Z += 1;

	if (Direction.Length() < EPSILON) return;

	Direction.Normalize();

	const float MoveSensitivity = Settings ? Settings->CameraMoveSensitivity : 1.f;
	const float CameraSpeed = (Settings ? Settings->CameraSpeed : 10.f) * MoveSensitivity;

	FVector FinalMove = Direction * CameraSpeed * DeltaTime;
	Camera->MoveLocal(FinalMove);
}

void FEditorViewportClient::Rotate(float DeltaTime)
{
	FVector Rotation = FVector(0, 0, 0);
	FVector MouseRotation = FVector(0, 0, 0);

	const float RotateSensitivity = Settings ? Settings->CameraRotateSensitivity : 1.f;
	const float AngleVelocity = (Settings ? Settings->CameraRotationSpeed : 60.f) * RotateSensitivity;
	if (InputSystem::Get().GetKey(VK_UP))
		Rotation.Z -= AngleVelocity;
	if (InputSystem::Get().GetKey(VK_LEFT))
		Rotation.Y -= AngleVelocity;
	if (InputSystem::Get().GetKey(VK_DOWN))
		Rotation.Z += AngleVelocity;
	if (InputSystem::Get().GetKey(VK_RIGHT))
		Rotation.Y += AngleVelocity;

	float MouseRotationSpeed = 0.15f * RotateSensitivity;

	if (bIsRightMouseDown)
	{
		float DeltaX = static_cast<float>(InputSystem::Get().MouseDeltaX());
		float DeltaY = static_cast<float>(InputSystem::Get().MouseDeltaY());

		MouseRotation.Y += DeltaX * MouseRotationSpeed; // yaw
		MouseRotation.Z += DeltaY * MouseRotationSpeed; // pitch

		MouseRotation.Y = Clamp(MouseRotation.Y, -89.0f, 89.0f);
		MouseRotation.Z = Clamp(MouseRotation.Z, -89.0f, 89.0f);

		Rotation *= DeltaTime;
		Camera->Rotate(Rotation.Y + MouseRotation.Y, Rotation.Z + MouseRotation.Z);
	}
}

void FEditorViewportClient::Zoom(float DeltaTime)
{
	const float ZoomSpeed = Settings ? Settings->CameraZoomSpeed : 300.f;

	float ScrollNotches = MouseWheelDelta;
	if (ScrollNotches != 0.0f) {
		if (Camera->IsOrthogonal()) {
			float NewWidth = Camera->GetOrthoWidth() - ScrollNotches * ZoomSpeed * DeltaTime;
			Camera->SetOrthoWidth(Clamp(NewWidth, 0.1f, 1000.0f));
		}
		else {
			float NewFOV = Camera->GetFOV() - ScrollNotches * ZoomSpeed * DeltaTime;
			Camera->SetFOV(Clamp(NewFOV, 1.f * DEG_TO_RAD, 90.0f * DEG_TO_RAD));
		}
	}
}

void FEditorViewportClient::TickInteraction(float DeltaTime)
{
	if (!Camera || !Gizmo || !World)
	{
		return;
	}

	Gizmo->ApplyScreenSpaceScaling(Camera->GetWorldLocation());

	if (InputSystem::Get().GetGuiInputState().bUsingMouse)
	{
		return;
	}

	const float ZoomSpeed = Settings ? Settings->CameraZoomSpeed : 300.f;

	float ScrollNotches = InputSystem::Get().GetScrollNotches();
	if (ScrollNotches != 0.0f) {
		if (Camera->IsOrthogonal()) {
			float NewWidth = Camera->GetOrthoWidth() - ScrollNotches * ZoomSpeed * DeltaTime;
			Camera->SetOrthoWidth(Clamp(NewWidth, 0.1f, 1000.0f));
		}
		else {
			float NewFOV = Camera->GetFOV() - ScrollNotches * ZoomSpeed * DeltaTime;
			Camera->SetFOV(Clamp(NewFOV, 1.f * DEG_TO_RAD, 90.0f * DEG_TO_RAD));
		}
	}

	//FRay Ray = Camera->DeprojectScreenToWorld(static_cast<float>(MousePoint.x), static_cast<float>(MousePoint.y), ViewportSize.X, ViewportSize.Y);
	//FHitResult HitResult;

	////Gizmo Hover
	//Gizmo->Raycast(Ray, HitResult);

	//if (InputSystem::Get().GetKeyDown(VK_LBUTTON))
	//{
	//	HandleDragStart(Ray);
	//}
	//else if (InputSystem::Get().GetLeftDragging())
	//{
	//	//	눌려있고, Holding되지 않았다면 다음 Loop부터 드래그 업데이트 시작
	//	if (Gizmo->IsPressedOnHandle() && !Gizmo->IsHolding())
	//	{
	//		Gizmo->SetHolding(true);
	//	}

	//	if (Gizmo->IsHolding())
	//	{
	//		Gizmo->UpdateDrag(Ray);
	//	}
	//}
	//else if (InputSystem::Get().GetLeftDragEnd())
	//{
	//	Gizmo->DragEnd();
	//}
}

void FEditorViewportClient::HandleDragStart(const FRay& Ray)
{
	FHitResult HitResult{};
	if (Gizmo->Raycast(Ray, HitResult))
	{
		Gizmo->SetPressedOnHandle(true);
		UE_LOG("Gizmo is Holding");
	}
	else
	{
		AActor* BestActor = nullptr;
		float ClosestDistance = FLT_MAX;

		for (AActor* Actor : World->GetActors())
		{
			if (!Actor || !Actor->GetRootComponent()) {
				continue;
			}
			//USceneComponent* RootComp = Actor->GetRootComponent();
			//if (!RootComp->IsA<UPrimitiveComponent>()) continue;

			for (auto* primitive : Actor->GetPrimitiveComponents())
			{
				UPrimitiveComponent* PrimitiveComp = static_cast<UPrimitiveComponent*>(primitive);

				HitResult = {};
				if (PrimitiveComp->Raycast(Ray, HitResult))
				{
					if (HitResult.Distance < ClosestDistance)
					{
						ClosestDistance = HitResult.Distance;
						BestActor = Actor;
					}
				}
			}
			
		}

		bool bCtrlHeld = InputSystem::Get().GetKey(VK_CONTROL);

		if (BestActor == nullptr)
		{
			if (!bCtrlHeld)
			{
				SelectionManager->ClearSelection();
			}
		}
		else
		{
			if (bCtrlHeld)
			{
				SelectionManager->ToggleSelect(BestActor);
			}
			else
			{
				SelectionManager->Select(BestActor);
			}
		}
	}
}
