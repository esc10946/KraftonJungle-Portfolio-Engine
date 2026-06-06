#include "Game/Lua/GameLuaBindings.h"

#include "sol/sol.hpp"

#include "Engine/Runtime/Engine.h"
#include "Engine/Runtime/EngineInitHooks.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimationManager.h"
#include "Animation/Montage/AnimMontage.h"
#include "Animation/Notify/AnimNotify_EnableAttack.h"
#include "Animation/Notify/AnimNotifyState_ParryWindow.h"
#include "Component/Camera/CameraComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Primitive/StaticMeshComponent.h"
#include "Engine/GameFramework/World.h"
#include "Engine/GameFramework/GameMode/PlayerController.h"
#include "Engine/GameFramework/Camera/PlayerCameraManager.h"
#include "Engine/Render/Types/MinimalViewInfo.h"
#include "Lua/LuaScriptManager.h"
#include "Core/ProjectSettings.h"
#include "Core/Logging/Log.h"
#include "Audio/AudioManager.h"
#include "Game/GameMode/AFinaleGameMode.h"
#include "Game/GameMode/GameState.h"
#include "Game/Leaderboard/LeaderboardStore.h"
#include "Engine/Core/Logging/Log.h"
#include "Engine/Viewport/GameViewportClient.h"
#include "Engine/Viewport/Viewport.h"

#include <algorithm>
#include <cmath>
#include <vector>




// ============================================================
// 게임-특화 Lua 바인딩 등록 위치 — 현재는 비어 있음.
//
// Engine 의 FLuaScriptManager 가 등록하는 일반 binding (AActor / APawn / FVector /
// UWorld / Anim 등) 만으로 동작하지 않는 game-specific usertype (ACarPawn /
// AGameStateXxx / 전용 enum 등) 이 도입되면 여기에 new_usertype 으로 추가한다.
//
// 호출 시점: UEngine::Init() 이 FLuaScriptManager::Initialize() 를 끝낸 직후.
// 등록은 EngineInitHooks 에 자동으로 걸려 GameEngine / EditorEngine 두 엔트리 모두
// 같은 바인딩이 적용된다 (PIE 호환).
// ============================================================
namespace
{
	constexpr float ScreenProjectionEpsilon = 1.0e-4f;

	struct FScreenProjectionResult
	{
		bool bVisible = false;
		float ScreenX = 0.0f;
		float ScreenY = 0.0f;
		float Distance = 0.0f;
		float ViewDepth = 0.0f;
		float ViewportWidth = 0.0f;
		float ViewportHeight = 0.0f;
		FVector CameraLocation = FVector::ZeroVector;
	};

	void ApplyActiveCameraLetterboxAspect(FMinimalViewInfo& POV, UWorld* World, float ViewportWidth, float ViewportHeight)
	{
		APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		APlayerCameraManager* CameraManager = PlayerController ? PlayerController->GetPlayerCameraManager() : nullptr;
		UCameraComponent* ActiveCamera = CameraManager ? CameraManager->GetActiveCamera() : nullptr;
		if (!ActiveCamera || ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
		{
			return;
		}

		const FCameraLetterboxState& Letterbox = ActiveCamera->GetLetterboxSettings();
		if (!Letterbox.bEnabled || Letterbox.Amount <= 0.0f)
		{
			return;
		}

		const float Thickness = std::clamp(Letterbox.Thickness * Letterbox.Amount, 0.0f, 0.49f);
		const float VisibleHeightScale = 1.0f - Thickness * 2.0f;
		if (VisibleHeightScale <= ScreenProjectionEpsilon)
		{
			return;
		}

		POV.AspectRatio = (ViewportWidth / ViewportHeight) / VisibleHeightScale;
	}

	bool ProjectActiveWorldToScreen(const FVector& WorldPosition, FScreenProjectionResult& OutResult)
	{
		if (!GEngine)
		{
			return false;
		}

		UWorld* World = GEngine->GetWorld();
		UGameViewportClient* ViewportClient = GEngine->GetGameViewportClient();
		FViewport* Viewport = ViewportClient ? ViewportClient->GetViewport() : nullptr;
		if (!World || !Viewport)
		{
			return false;
		}

		const float ViewportWidth = static_cast<float>(Viewport->GetWidth());
		const float ViewportHeight = static_cast<float>(Viewport->GetHeight());
		OutResult.ViewportWidth = ViewportWidth;
		OutResult.ViewportHeight = ViewportHeight;
		if (ViewportWidth <= 1.0f || ViewportHeight <= 1.0f)
		{
			return false;
		}

		FMinimalViewInfo POV;
		if (!World->GetActivePOV(POV))
		{
			return false;
		}

		const FVector ToPoint = WorldPosition - POV.Location;
		const FVector ViewForward = POV.Rotation.GetForwardVector();
		OutResult.CameraLocation = POV.Location;
		OutResult.Distance = ToPoint.Length();
		OutResult.ViewDepth = ToPoint.Dot(ViewForward);
		if (OutResult.ViewDepth <= 0.0f)
		{
			return false;
		}

		ApplyActiveCameraLetterboxAspect(POV, World, ViewportWidth, ViewportHeight);

		const FMatrix ViewProj = POV.CalculateViewProjectionMatrix();
		const float ClipX =
			WorldPosition.X * ViewProj.M[0][0] +
			WorldPosition.Y * ViewProj.M[1][0] +
			WorldPosition.Z * ViewProj.M[2][0] +
			ViewProj.M[3][0];
		const float ClipY =
			WorldPosition.X * ViewProj.M[0][1] +
			WorldPosition.Y * ViewProj.M[1][1] +
			WorldPosition.Z * ViewProj.M[2][1] +
			ViewProj.M[3][1];
		const float ClipZ =
			WorldPosition.X * ViewProj.M[0][2] +
			WorldPosition.Y * ViewProj.M[1][2] +
			WorldPosition.Z * ViewProj.M[2][2] +
			ViewProj.M[3][2];
		const float ClipW =
			WorldPosition.X * ViewProj.M[0][3] +
			WorldPosition.Y * ViewProj.M[1][3] +
			WorldPosition.Z * ViewProj.M[2][3] +
			ViewProj.M[3][3];
		if (std::abs(ClipW) <= ScreenProjectionEpsilon)
		{
			return false;
		}

		const float NdcX = ClipX / ClipW;
		const float NdcY = ClipY / ClipW;
		const float NdcZ = ClipZ / ClipW;
		if (NdcX < -1.0f || NdcX > 1.0f || NdcY < -1.0f || NdcY > 1.0f || NdcZ < 0.0f || NdcZ > 1.0f)
		{
			return false;
		}

		OutResult.ScreenX = (NdcX * 0.5f + 0.5f) * ViewportWidth;
		OutResult.ScreenY = (0.5f - NdcY * 0.5f) * ViewportHeight;
		OutResult.bVisible = true;
		return true;
	}

	sol::table MakeProjectWorldToScreenResult(sol::this_state State, const FVector& WorldPosition)
	{
		sol::state_view Lua(State);
		sol::table Result = Lua.create_table();

		FScreenProjectionResult Projection;
		const bool bVisible = ProjectActiveWorldToScreen(WorldPosition, Projection);

		Result["Visible"] = bVisible;
		Result["X"] = bVisible ? Projection.ScreenX : 0.0f;
		Result["Y"] = bVisible ? Projection.ScreenY : 0.0f;
		Result["Distance"] = Projection.Distance;
		Result["ViewDepth"] = Projection.ViewDepth;
		Result["ViewportWidth"] = Projection.ViewportWidth;
		Result["ViewportHeight"] = Projection.ViewportHeight;
		Result["CameraLocation"] = Projection.CameraLocation;
		Result["CameraX"] = Projection.CameraLocation.X;
		Result["CameraY"] = Projection.CameraLocation.Y;
		Result["CameraZ"] = Projection.CameraLocation.Z;
		return Result;
	}
}

void RegisterGameLuaBindings(sol::state& Lua)
{
	// "Engine" 테이블은 엔진 측 바인딩(FLuaScriptManager)이 이미 만들어 두지만,
	// 등록 순서에 의존하지 않도록 없으면 생성한다.
	sol::table Engine = Lua["Engine"].valid() ? Lua["Engine"] : Lua.create_named_table("Engine");

	// Engine.OpenScene("MapName" 또는 "Scene/Foo.Scene") — 다음 frame Tick 끝에서
	// active world 를 destroy 하고 해당 scene 으로 교체한다. World->Tick 바깥에서
	// 일어나므로 버튼 클릭(Lua) 콜백에서 호출해도 use-after-free 가 없다.
	Engine.set_function(
		"OpenScene",
		[](const FString& SceneNameOrPath)
		{
			if (GEngine)
			{
				GEngine->RequestTransitionToScene(SceneNameOrPath);
			}
		}
	);
	Engine.set_function(
		"ProjectWorldToScreen",
		sol::overload(
			[](sol::this_state State, const FVector& WorldPosition) -> sol::table
			{
				return MakeProjectWorldToScreenResult(State, WorldPosition);
			},
			[](sol::this_state State, float X, float Y, float Z) -> sol::table
			{
				return MakeProjectWorldToScreenResult(State, FVector(X, Y, Z));
			}
		)
	);

	sol::table Animation = Lua["Animation"].valid() ? Lua["Animation"] : Lua.create_named_table("Animation");
	Animation.set_function(
		"LoadMontage",
		[](const FString& Path) -> UAnimMontage*
		{
			if (Path.empty() || Path == "None")
			{
				return nullptr;
			}
			return FAnimationManager::Get().LoadMontage(Path);
		}
	);
	Animation.set_function(
		"ConsumeEnableAttack",
		[](UAnimInstance* AnimInstance) -> bool
		{
			return UAnimNotify_EnableAttack::ConsumeEnableAttack(AnimInstance);
		}
	);
	Animation.set_function(
		"IsParryWindowActive",
		[](UAnimInstance* AnimInstance) -> bool
		{
			return UAnimNotifyState_ParryWindow::IsParryWindowActive(AnimInstance);
		}
	);
	Animation.set_function(
		"ReportSuccessfulParry",
		[](UAnimInstance* AnimInstance) -> bool
		{
			return UAnimNotifyState_ParryWindow::ReportSuccessfulParry(AnimInstance);
		}
	);
	Animation.set_function(
		"ConsumeSuccessfulParry",
		[](UAnimInstance* AnimInstance) -> bool
		{
			return UAnimNotifyState_ParryWindow::ConsumeSuccessfulParry(AnimInstance);
		}
	);

	sol::table Equipment = Lua["Equipment"].valid() ? Lua["Equipment"] : Lua.create_named_table("Equipment");
	Equipment.set_function(
		"AttachStaticMeshToSocket",
		[](AActor* Owner, const FString& MeshPath, const FString& SocketName, const FVector& RelativeScale) -> UStaticMeshComponent*
		{
			if (!IsValid(Owner) || MeshPath.empty() || MeshPath == "None")
			{
				UE_LOG("[Equipment] AttachStaticMeshToSocket failed: invalid owner or mesh path");
				return nullptr;
			}

			USkeletalMeshComponent* OwnerMesh = Owner->GetComponentByClass<USkeletalMeshComponent>();
			if (!IsValid(OwnerMesh))
			{
				UE_LOG("[Equipment] AttachStaticMeshToSocket failed: owner has no skeletal mesh");
				return nullptr;
			}

			if (!OwnerMesh->HasSocket(FName(SocketName)))
			{
				UE_LOG("[Equipment] AttachStaticMeshToSocket failed: socket not found. Socket='%s'", SocketName.c_str());
				return nullptr;
			}

			UStaticMeshComponent* WeaponMesh = Owner->AddComponent<UStaticMeshComponent>();
			if (!IsValid(WeaponMesh))
			{
				UE_LOG("[Equipment] AttachStaticMeshToSocket failed: could not create static mesh component");
				return nullptr;
			}

			if (!WeaponMesh->SetStaticMeshByPath(MeshPath))
			{
				UE_LOG("[Equipment] AttachStaticMeshToSocket failed: could not load static mesh. Mesh='%s'", MeshPath.c_str());
				Owner->RemoveComponent(WeaponMesh);
				return nullptr;
			}

			WeaponMesh->AttachToComponent(OwnerMesh, FName(SocketName));
			WeaponMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
			WeaponMesh->SetRelativeRotation(FVector(0.0f, 0.0f, 0.0f));
			WeaponMesh->SetRelativeScale(RelativeScale);
			return WeaponMesh;
		}
	);

	// Game.TogglePause — pause key entry point. Resolves the active GameMode and
	// lets it decide pause vs. resume from its own phase.
	sol::table Game = Lua["Game"].valid() ? Lua["Game"] : Lua.create_named_table("Game");
	Game.set_function(
		"TogglePause",
		[]()
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				GM->TogglePause();
			}
		}
	);
	Game.set_function(
		"QuitToTitle",
		[]()
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				GM->OnGameQuit();
			}
		}
	);
	// Phase string (e.g. "Playing", "Dead", "Defeated") — GameFlowController
	// polls this each Tick to drive phase-dependent UI (death overlay, etc.).
	Game.set_function(
		"GetPhase",
		[]() -> FString
		{
			if (!GEngine || !GEngine->GetWorld()) return "None";
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				if (AFinaleGameState* GS = Cast<AFinaleGameState>(GM->GetGameState()))
				{
					return GS->GetPhaseString();
				}
			}
			return "None";
		}
	);
	Game.set_function(
		"PlayerDeath",
		[]() 
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
				GM->OnPlayerDeath();
		});
	Game.set_function(
		"Revive",
		[]()
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				GM->OnPlayerRevive();
			}
		}
	);
	Game.set_function(
		"Defeated",
		[]()
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				GM->OnPlayerDefeated();
			}
		}
	);
	// Current camera fade alpha (0..1). Lets UI track the engine post-process
	// fade exactly (e.g. redden the death icon in lockstep with the fade-out).
	Game.set_function(
		"GetCameraFade",
		[]() -> float
		{
			if (!GEngine || !GEngine->GetWorld()) return 0.0f;
			APlayerController* PC = GEngine->GetWorld()->GetFirstPlayerController();
			APlayerCameraManager* Manager = PC ? PC->GetPlayerCameraManager() : nullptr;
			return Manager ? Manager->GetFadeAmount() : 0.0f;
		}
	);
	// Fade (black) from an explicit alpha — unlike CameraManager.FadeIn which
	// always starts at 1.0. Lets a revive continue from the death fade's current
	// alpha instead of snapping to black first.
	Game.set_function(
		"CameraFade",
		[](float FromAlpha, float ToAlpha, float Duration)
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			APlayerController* PC = GEngine->GetWorld()->GetFirstPlayerController();
			APlayerCameraManager* Manager = PC ? PC->GetPlayerCameraManager() : nullptr;
			if (Manager)
			{
				Manager->StartCameraFade(FromAlpha, ToAlpha, Duration, FLinearColor::Black(), false, true);
			}
		}
	);
	// Game.ViewLeaderboard — victory/defeat LEADERBOARD button. Drives the phase
	// to Leaderboard (gated in OnLeaderBoardView); the leaderboard screen is TODO.
	Game.set_function(
		"ViewLeaderboard",
		[]()
		{
			if (!GEngine || !GEngine->GetWorld()) return;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				GM->OnLeaderBoardView();
			}
		}
	);
	// Game.GetActiveTime — seconds of Playing time elapsed up to victory. The
	// leaderboard reads this at submit (the run is frozen by then) for the record.
	Game.set_function(
		"GetActiveTime",
		[]() -> float
		{
			if (!GEngine || !GEngine->GetWorld()) return 0.0f;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				return GM->GetActiveTime();
			}
			return 0.0f;
		}
	);
	// Game.GetReviveCount — number of revives the player used this run.
	Game.set_function(
		"GetReviveCount",
		[]() -> int
		{
			if (!GEngine || !GEngine->GetWorld()) return 0;
			if (AFinaleGameMode* GM = Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode()))
			{
				return static_cast<int>(GM->GetReviveCount());
			}
			return 0;
		}
	);

	// ============================================================
	// Leaderboard — file-backed top-N store (Saves/Leaderboard.txt). The UI is an
	// RmlUi overlay driven from GameFlowController.lua: submit on victory (writes a
	// record), view-only elsewhere (defeat/pause/title). Lua has no file I/O, so
	// persistence lives in C++ (GameLeaderboard, see LeaderboardStore.h).
	// ============================================================
	sol::table Leaderboard = Lua["Leaderboard"].valid() ? Lua["Leaderboard"] : Lua.create_named_table("Leaderboard");
	Leaderboard.set_function(
		"Submit",
		[](const FString& Name, float TimeSec, int Revives)
		{
			GameLeaderboard::Submit(Name, TimeSec, Revives);
		}
	);
	// Returns a 1-based array of { name, time, revives } sorted best-first.
	Leaderboard.set_function(
		"GetEntries",
		[](sol::this_state S) -> sol::table
		{
			sol::state_view View(S);
			sol::table Out = View.create_table();
			std::vector<GameLeaderboard::FEntry> Entries = GameLeaderboard::Load();
			int Index = 1;
			for (const GameLeaderboard::FEntry& E : Entries)
			{
				sol::table Row = View.create_table();
				Row["name"]    = E.Name;
				Row["time"]    = E.TimeSec;
				Row["revives"] = E.Revives;
				Out[Index++]   = Row;
			}
			return Out;
		}
	);

	// ============================================================
	// Debug flow triggers (dev only)
	// Bound to number keys in GameFlowController.lua to drive the macro state
	// machine without real combat/gameplay. Each fires one flow event on the
	// active GameMode; the handlers self-gate on EGamePhase, so a key pressed
	// in the wrong phase is a harmless no-op. Strip the DEBUG_KEYS block in the
	// Lua controller (or this table) before shipping.
	// ============================================================
	auto ResolveFinaleGM = []() -> AFinaleGameMode*
	{
		if (!GEngine || !GEngine->GetWorld()) return nullptr;
		return Cast<AFinaleGameMode>(GEngine->GetWorld()->GetGameMode());
	};

	sol::table Debug = Lua["Debug"].valid() ? Lua["Debug"] : Lua.create_named_table("Debug");
	Debug.set_function("PlayerDeath",  [ResolveFinaleGM]() { if (AFinaleGameMode* GM = ResolveFinaleGM()) GM->OnPlayerDeath(); });
	Debug.set_function("PlayerRevive", [ResolveFinaleGM]() { if (AFinaleGameMode* GM = ResolveFinaleGM()) GM->OnPlayerRevive(); });
	Debug.set_function("BossSlain",   [ResolveFinaleGM]() { if (AFinaleGameMode* GM = ResolveFinaleGM()) GM->OnBossSlain(FName("DebugBoss")); });
	Debug.set_function("Victory",     [ResolveFinaleGM]() { if (AFinaleGameMode* GM = ResolveFinaleGM()) GM->OnVictory(); });
	Debug.set_function("Cutscene",    [ResolveFinaleGM]() { if (AFinaleGameMode* GM = ResolveFinaleGM()) GM->OnEnteringCutscene(); });
	Debug.set_function("Leaderboard", [ResolveFinaleGM]() { if (AFinaleGameMode* GM = ResolveFinaleGM()) GM->OnLeaderBoardView(); });
	Debug.set_function("PrintPhase",  [ResolveFinaleGM]()
	{
		if (AFinaleGameMode* GM = ResolveFinaleGM())
		{
			if (AFinaleGameState* GS = Cast<AFinaleGameState>(GM->GetGameState()))
			{
				UE_LOG("[Debug] Current phase: %s", GS->GetPhaseString().c_str());
			}
		}
	});

	// ---- Options ----
	// 런타임 source of truth 는 FProjectSettings (in-memory singleton).
	// SetMasterVolume: 메모리 갱신 + FAudioManager 에 즉시 적용(apply-on-change).
	// Save: ProjectSettings.ini 에 영속화 (load→mutate→save 라 다른 설정을 안 덮어씀).
	sol::table Options = Lua.create_named_table("Options");

	Options.set_function(
		"GetMasterVolume",
		[]() -> float
		{
			return FProjectSettings::Get().Audio.MasterVolume;
		}
	);

	Options.set_function(
		"SetMasterVolume",
		[](float Volume)
		{
			const float Clamped = std::clamp(Volume, 0.0f, 1.0f);
			FProjectSettings::Get().Audio.MasterVolume = Clamped;
			FAudioManager::Get().SetMasterVolume(Clamped);
		}
	);

	Options.set_function(
		"Save",
		[]()
		{
			FProjectSettings::Get().SaveToFile(FProjectSettings::GetDefaultPath());
		}
	);
}

// 자기-등록 — Editor / Game 측이 RegisterGameLuaBindings 함수명을 모르고도
// FEngineInitHooks::RunAll() 한 번이면 호출되도록 static initializer 로 등록.
namespace
{
	void RunRegisterGameLuaBindings()
	{
		RegisterGameLuaBindings(FLuaScriptManager::GetState());
	}

	struct GameLuaBindingsAutoReg
	{
		GameLuaBindingsAutoReg() { FEngineInitHooks::Register(&RunRegisterGameLuaBindings); }
	};

	static GameLuaBindingsAutoReg gAutoReg;
}
