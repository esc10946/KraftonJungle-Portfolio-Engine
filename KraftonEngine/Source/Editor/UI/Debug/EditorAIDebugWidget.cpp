#include "Editor/UI/Debug/EditorAIDebugWidget.h"

#include "Editor/EditorEngine.h"
#include "Editor/Selection/SelectionManager.h"

#include "ImGui/imgui.h"

#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/AIDecisionTraceComponent.h"
#include "Component/AI/AIPerceptionComponent.h"
#include "Component/AI/AwarenessComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/Character/CharacterStateMachineComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/CombatMoveComponent.h"
#include "Component/Combat/CombatTypes.h"
#include "Component/Combat/HealthComponent.h"
#include "Debug/DrawDebugHelpers.h"
#include "Core/Types/EngineTypes.h"
#include "Math/Vector.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/EnemyCharacter.h"
#include "Object/Object.h"
#include "Object/Reflection/UClass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
	constexpr int32 kHistoryCapacity = 240;
	constexpr float kTimelineWindow  = 12.0f; // 타임라인 가시 구간(초)
	constexpr int32 kMaxSegments     = 64;

	void PushRolling(TArray<float>& History, float Value)
	{
		History.push_back(Value);
		while (static_cast<int32>(History.size()) > kHistoryCapacity)
		{
			History.erase(History.begin());
		}
	}

	// 안정적 색 팔레트 — 상태/후보 key 로 인덱싱.
	ImU32 PaletteColor(int32 Key)
	{
		static const ImU32 kPalette[] = {
			IM_COL32( 90, 170, 230, 255), IM_COL32(120, 200, 120, 255), IM_COL32(230, 200,  90, 255),
			IM_COL32(230, 120, 110, 255), IM_COL32(180, 130, 220, 255), IM_COL32(120, 205, 205, 255),
			IM_COL32(225, 150,  90, 255), IM_COL32(170, 170, 170, 255), IM_COL32(210, 110, 170, 255),
			IM_COL32(150, 190,  90, 255), IM_COL32(110, 150, 235, 255), IM_COL32(200,  90,  90, 255),
		};
		const int32 N = static_cast<int32>(sizeof(kPalette) / sizeof(kPalette[0]));
		return kPalette[((Key % N) + N) % N];
	}

	// FName → 안정적 정수 key(FNV-1a) — HFSM 상태별 색 고정용.
	int32 NameKey(const FName& Name)
	{
		if (!Name.IsValid())
		{
			return 0;
		}
		const FString S = Name.ToString();
		uint32 H = 2166136261u;
		for (const char* P = S.c_str(); *P; ++P)
		{
			H ^= static_cast<uint8>(*P);
			H *= 16777619u;
		}
		return static_cast<int32>(H & 0x7fffffff);
	}
}

// FStateSegment 는 위젯 private 중첩 타입 — 타임라인/링은 Render 내부 람다로 그린다.

void FEditorAIDebugWidget::ResetTracking()
{
	HealthHistory.clear();
	PostureHistory.clear();
	SuspicionHistory.clear();
	HfsmSegments.clear();
	AwareSegments.clear();
	LastHfsmState  = FName::None;
	LastAwareState = -1;
	HfsmSegStart   = 0.0f;
	AwareSegStart  = 0.0f;
	TimelineClock  = 0.0f;
}

void FEditorAIDebugWidget::SampleHistory(AEnemyCharacter* Enemy, float DeltaTime)
{
	TimelineClock += DeltaTime;

	float HealthRatio  = 1.0f;
	float PostureRatio = 1.0f;
	if (UHealthComponent* Health = Enemy->GetHealthComponent())
	{
		HealthRatio = Health->GetHealthRatio();
	}
	if (UCombatStateComponent* Combat = Enemy->GetCombatStateComponent())
	{
		PostureRatio = Combat->GetPoiseRatio();
	}
	PushRolling(HealthHistory, HealthRatio);
	PushRolling(PostureHistory, PostureRatio);

	float Suspicion = 0.0f;
	int32 AwareState = -1;
	if (UAwarenessComponent* Aware = Enemy->GetAwareness())
	{
		Suspicion  = Aware->GetSuspicion();
		AwareState = Aware->GetAwarenessStateInt();
	}
	PushRolling(SuspicionHistory, Suspicion);

	// ── HFSM 상태 전이 타임라인 샘플링 ──
	if (UCharacterStateMachineComponent* SM = Enemy->GetStateMachine())
	{
		const FName St = SM->GetState();
		if (!(St == LastHfsmState))
		{
			if (LastHfsmState.IsValid())
			{
				FStateSegment Seg;
				Seg.Label    = LastHfsmState;
				Seg.Start    = HfsmSegStart;
				Seg.End      = TimelineClock;
				Seg.ColorKey = NameKey(LastHfsmState);
				HfsmSegments.push_back(Seg);
				while (static_cast<int32>(HfsmSegments.size()) > kMaxSegments)
				{
					HfsmSegments.erase(HfsmSegments.begin());
				}
			}
			LastHfsmState = St;
			HfsmSegStart  = TimelineClock;
		}
	}

	// ── Awareness 상태 전이 타임라인 샘플링 ──
	if (AwareState != LastAwareState)
	{
		if (LastAwareState >= 0 && static_cast<uint32>(LastAwareState) < GAwarenessStateCount)
		{
			FStateSegment Seg;
			Seg.Label    = FName(GAwarenessStateNames[LastAwareState]);
			Seg.Start    = AwareSegStart;
			Seg.End      = TimelineClock;
			Seg.ColorKey = LastAwareState;
			AwareSegments.push_back(Seg);
			while (static_cast<int32>(AwareSegments.size()) > kMaxSegments)
			{
				AwareSegments.erase(AwareSegments.begin());
			}
		}
		LastAwareState = AwareState;
		AwareSegStart  = TimelineClock;
	}
}

void FEditorAIDebugWidget::DrawSensorOverlay(AEnemyCharacter* Enemy)
{
	UWorld* World = Enemy->GetWorld();
	if (!World)
	{
		return;
	}

	UAIPerceptionComponent* Perception = Enemy->GetPerception();
	const float Fov   = Perception ? Perception->FieldOfViewDegrees : 120.0f;
	const float Range = Perception ? Perception->SightRange         : 45.0f;
	const float Eye   = Perception ? Perception->EyeHeight          : 1.6f;
	const float Half  = Fov * 0.5f;

	const FVector Up(0.0f, 0.0f, 1.0f);
	const FVector EyePos = Enemy->GetActorLocation() + Up * Eye;
	FVector Fwd = Enemy->GetActorForward();
	Fwd.Z = 0.0f;
	Fwd = Fwd.IsNearlyZero() ? FVector(1.0f, 0.0f, 0.0f) : Fwd.Normalized();

	auto RotZ = [](const FVector& V, float Deg) -> FVector
	{
		const float R = Deg * 0.01745329252f;
		const float C = cosf(R);
		const float S = sinf(R);
		return FVector(V.X * C - V.Y * S, V.X * S + V.Y * C, V.Z);
	};

	// 시야 콘(부채꼴) 외곽선.
	const int32 Seg = 14;
	const FColor ConeCol = FColor::Yellow();
	FVector Prev = EyePos + RotZ(Fwd, -Half) * Range;
	DrawDebugLine(World, EyePos, Prev, ConeCol);
	for (int32 i = 1; i <= Seg; ++i)
	{
		const float A = -Half + Fov * (static_cast<float>(i) / static_cast<float>(Seg));
		const FVector Cur = EyePos + RotZ(Fwd, A) * Range;
		DrawDebugLine(World, Prev, Cur, ConeCol);
		Prev = Cur;
	}
	DrawDebugLine(World, EyePos, EyePos + RotZ(Fwd, Half) * Range, ConeCol);

	// 타깃 LOS 광선 — 통하면 초록, 막히면 빨강.
	if (UEnemyAIBrainComponent* Brain = Enemy->GetAIBrainComponent())
	{
		if (AActor* Target = Brain->GetTarget())
		{
			const FVector TargetEye = Target->GetActorLocation() + Up * Eye;
			const bool bClear = Perception && Perception->HasLineOfSight();
			DrawDebugLine(World, EyePos, TargetEye, bClear ? FColor::Green() : FColor::Red());
		}
	}

	// 기억 위치 마커.
	if (UAIBlackboardComponent* BB = Enemy->GetBlackboard())
	{
		const FVector LastSeen = BB->GetLastSeenLocation();
		if (!LastSeen.IsNearlyZero())
		{
			DrawDebugSphere(World, LastSeen + Up * 0.2f, 0.4f, 10, FColor::Blue());
		}
	}
	if (UAwarenessComponent* Aware = Enemy->GetAwareness())
	{
		const FVector Investigate = Aware->GetInvestigateLocation();
		if (!Investigate.IsNearlyZero())
		{
			DrawDebugSphere(World, Investigate + Up * 0.2f, 0.5f, 10, FColor(255, 140, 0));
		}
	}
}

void FEditorAIDebugWidget::Render(float DeltaTime)
{
	ImGui::SetNextWindowSize(ImVec2(460.0f, 720.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("AI Debug"))
	{
		ImGui::End();
		return;
	}

	AActor* PrimaryActor = EditorEngine
		? EditorEngine->GetSelectionManager().GetPrimarySelection()
		: nullptr;

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(PrimaryActor);
	if (!Enemy)
	{
		ImGui::TextDisabled("Select an AEnemyCharacter to inspect its combat AI.");
		ResetTracking();
		LastTrackedActor = nullptr;
		ImGui::End();
		return;
	}

	if (LastTrackedActor != static_cast<const void*>(Enemy))
	{
		ResetTracking();
		LastTrackedActor = static_cast<const void*>(Enemy);
	}
	SampleHistory(Enemy, DeltaTime);

	UAIBlackboardComponent*    BB         = Enemy->GetBlackboard();
	UAIPerceptionComponent*    Perception = Enemy->GetPerception();
	UAIDecisionTraceComponent* Trace      = Enemy->GetDecisionTrace();
	UEnemyAIBrainComponent*    Brain      = Enemy->GetAIBrainComponent();
	UCombatMoveComponent*      Move       = Enemy->GetCombatMove();
	UCombatStateComponent*     Combat     = Enemy->GetCombatStateComponent();
	UAwarenessComponent*       Aware      = Enemy->GetAwareness();

	ImGui::Text("Target: %s", Enemy->GetName().c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("(phase %d)", Enemy->GetAIPhase());
	if (UCharacterStateMachineComponent* SM = Enemy->GetStateMachine())
	{
		ImGui::Text("HFSM State: %s   (t=%.2fs)", SM->GetState().ToString().c_str(), SM->GetTimeInState());
	}
	ImGui::Checkbox("Viewport sensor overlay (vision cone / LOS / memory)", &bSensorOverlay);
	if (bSensorOverlay)
	{
		DrawSensorOverlay(Enemy);
	}
	ImGui::Separator();

	// ── ① 결정 점수 막대(전 후보 히트맵) ─────────────────────────────────────────
	if (ImGui::CollapsingHeader("Decision Scores", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Trace)
		{
			const TArray<FDecisionCandidate>& Candidates = Trace->GetLastCandidates();
			if (Candidates.empty())
			{
				ImGui::TextDisabled("No scored candidates yet (engage in combat).");
			}
			else
			{
				const FName Chosen = Trace->GetLastChosen();
				ImGui::TextDisabled("state: %s    chosen: %s    candidates: %d",
					Trace->GetLastState().IsValid() ? Trace->GetLastState().ToString().c_str() : "-",
					Chosen.IsValid() ? Chosen.ToString().c_str() : "(none)",
					static_cast<int>(Candidates.size()));

				float MaxScore = 0.0f;
				for (const FDecisionCandidate& C : Candidates)
				{
					MaxScore = std::max(MaxScore, C.Score);
				}
				if (MaxScore <= 0.0f)
				{
					MaxScore = 1.0f;
				}

				int32 Shown = 0;
				for (const FDecisionCandidate& C : Candidates)
				{
					if (Shown++ >= 12)
					{
						break;
					}
					const bool bChosen = (C.Name == Chosen);
					const ImVec4 Col = bChosen ? ImVec4(0.28f, 0.78f, 0.36f, 1.0f)
											   : ImVec4(0.38f, 0.50f, 0.72f, 1.0f);
					char Overlay[80];
					std::snprintf(Overlay, sizeof(Overlay), "%s  %.2f", C.Name.ToString().c_str(), C.Score);
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col);
					ImGui::ProgressBar(C.Score / MaxScore, ImVec2(-1.0f, 0.0f), Overlay);
					ImGui::PopStyleColor();
				}
			}
		}
		else
		{
			ImGui::TextDisabled("No decision trace component.");
		}
	}

	// ── ② 의심도(Suspicion) 시계열 + 임계선 ──────────────────────────────────────
	if (ImGui::CollapsingHeader("Awareness / Suspicion", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Aware)
		{
			const int32 StateIdx = Aware->GetAwarenessStateInt();
			const char* StateName = (StateIdx >= 0 && static_cast<uint32>(StateIdx) < GAwarenessStateCount)
				? GAwarenessStateNames[StateIdx] : "?";
			static const ImVec4 kAwareColors[] = {
				ImVec4(0.60f, 0.60f, 0.60f, 1.0f), // Unaware
				ImVec4(0.90f, 0.80f, 0.30f, 1.0f), // Suspicious
				ImVec4(0.95f, 0.55f, 0.20f, 1.0f), // Investigating
				ImVec4(0.92f, 0.30f, 0.25f, 1.0f), // Alert
				ImVec4(0.35f, 0.80f, 0.85f, 1.0f), // Searching
				ImVec4(0.40f, 0.55f, 0.90f, 1.0f), // Returning
			};
			const ImVec4 SC = (StateIdx >= 0 && static_cast<uint32>(StateIdx) < GAwarenessStateCount)
				? kAwareColors[StateIdx] : ImVec4(1, 1, 1, 1);
			ImGui::TextColored(SC, "State: %s", StateName);
			ImGui::SameLine();
			ImGui::Text("   Suspicion: %.2f", Aware->GetSuspicion());

			const int32 SndClass = Aware->GetLastSoundClass();
			if (SndClass >= 0 && static_cast<uint32>(SndClass) < GAISoundClassCount)
			{
				const bool bImportant = (SndClass == static_cast<int32>(EAISoundClass::Important));
				ImGui::TextColored(bImportant ? ImVec4(0.95f, 0.55f, 0.20f, 1.0f) : ImVec4(0.55f, 0.65f, 0.80f, 1.0f),
					"Last heard sound: %s", GAISoundClassNames[SndClass]);
			}

			char Overlay[24];
			std::snprintf(Overlay, sizeof(Overlay), "%.2f", Aware->GetSuspicion());
			ImGui::PlotLines("##suspicion", SuspicionHistory.data(), static_cast<int>(SuspicionHistory.size()),
				0, Overlay, 0.0f, 1.0f, ImVec2(-1.0f, 64.0f));

			// 임계선(수상함/확신)을 플롯 위에 덧그린다.
			const ImVec2 RMin = ImGui::GetItemRectMin();
			const ImVec2 RMax = ImGui::GetItemRectMax();
			ImDrawList* DL = ImGui::GetWindowDrawList();
			auto YFor = [&](float V) { return RMax.y - V * (RMax.y - RMin.y); };
			const float YS = YFor(std::min(1.0f, Aware->SuspiciousThreshold));
			const float YC = YFor(std::min(1.0f, Aware->ConfirmThreshold));
			DL->AddLine(ImVec2(RMin.x, YS), ImVec2(RMax.x, YS), IM_COL32(230, 200, 90, 150));
			DL->AddLine(ImVec2(RMin.x, YC), ImVec2(RMax.x, YC), IM_COL32(230, 110, 90, 180));
			ImGui::TextDisabled("dashes: Suspicious >= %.2f (amber), Confirm >= %.2f (red)",
				Aware->SuspiciousThreshold, Aware->ConfirmThreshold);
		}
		else
		{
			ImGui::TextDisabled("No awareness component.");
		}
	}

	// ── ③ 상태 전이 타임라인(HFSM + Awareness) ───────────────────────────────────
	if (ImGui::CollapsingHeader("State Timeline", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto DrawTimeline = [&](const char* Id, const TArray<FStateSegment>& Segs,
								 const FName& LiveLabel, float LiveStart, int32 LiveKey)
		{
			const float H = 24.0f;
			float W = ImGui::GetContentRegionAvail().x;
			if (W < 1.0f)
			{
				W = 1.0f;
			}
			const ImVec2 P0 = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton(Id, ImVec2(W, H));
			const ImVec2 P1(P0.x + W, P0.y + H);
			ImDrawList* DL = ImGui::GetWindowDrawList();
			DL->AddRectFilled(P0, P1, IM_COL32(24, 24, 28, 255));

			const float Now = TimelineClock;
			const float T0  = Now - kTimelineWindow;
			auto XFor = [&](float T) { float F = (T - T0) / kTimelineWindow; F = F < 0 ? 0 : (F > 1 ? 1 : F); return P0.x + F * W; };
			auto DrawSeg = [&](const FName& Label, int32 Key, float S, float E)
			{
				if (E < T0)
				{
					return;
				}
				float Xs = XFor(S);
				float Xe = XFor(E);
				if (Xe - Xs < 1.0f)
				{
					Xe = Xs + 1.0f;
				}
				DL->AddRectFilled(ImVec2(Xs, P0.y + 1.0f), ImVec2(Xe, P1.y - 1.0f), PaletteColor(Key));
				if (Xe - Xs > 34.0f && Label.IsValid())
				{
					const FString L = Label.ToString();
					DL->PushClipRect(ImVec2(Xs, P0.y), ImVec2(Xe, P1.y), true);
					DL->AddText(ImVec2(Xs + 3.0f, P0.y + 4.0f), IM_COL32(20, 20, 22, 255), L.c_str());
					DL->PopClipRect();
				}
			};
			for (const FStateSegment& Seg : Segs)
			{
				DrawSeg(Seg.Label, Seg.ColorKey, Seg.Start, Seg.End);
			}
			DrawSeg(LiveLabel, LiveKey, LiveStart, Now); // 진행 중 구간
			DL->AddRect(P0, P1, IM_COL32(80, 80, 86, 255));
		};

		ImGui::TextDisabled("HFSM (last %.0fs)", kTimelineWindow);
		DrawTimeline("hfsm_timeline", HfsmSegments, LastHfsmState, HfsmSegStart, NameKey(LastHfsmState));
		ImGui::Spacing();
		const FName AwareLive = (LastAwareState >= 0 && static_cast<uint32>(LastAwareState) < GAwarenessStateCount)
			? FName(GAwarenessStateNames[LastAwareState]) : FName::None;
		ImGui::TextDisabled("Awareness (last %.0fs)", kTimelineWindow);
		DrawTimeline("aware_timeline", AwareSegments, AwareLive, AwareSegStart, LastAwareState);
	}

	// ── ④ 스쿼드 공격슬롯 링 ────────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Squad / Attack Slots", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (BB)
		{
			const int32 Slot     = static_cast<int32>(BB->GetFloat(FName("SquadSlot")) + 0.5f);
			const int32 Count    = static_cast<int32>(BB->GetFloat(FName("EngagerCount")) + 0.5f);
			const int32 Active   = static_cast<int32>(BB->GetFloat(FName("ActiveAttackers")) + 0.5f);
			const int32 Lod      = static_cast<int32>(BB->GetFloat(FName("LOD")) + 0.5f);
			const bool  bToken   = BB->GetBool(FName("HoldingToken"));

			ImGui::Text("slot %d / %d engagers    active attackers: %d    LOD %d    token: %s",
				Slot, Count, Active, Lod, bToken ? "HELD" : "-");

			const float H = 132.0f;
			float W = ImGui::GetContentRegionAvail().x;
			if (W < 1.0f)
			{
				W = 1.0f;
			}
			const ImVec2 P0 = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("squad_ring", ImVec2(W, H));
			ImDrawList* DL = ImGui::GetWindowDrawList();
			const ImVec2 C(P0.x + W * 0.5f, P0.y + H * 0.5f);
			const float R = H * 0.36f;

			DL->AddCircle(C, R, IM_COL32(70, 70, 78, 180), 48);
			DL->AddCircleFilled(C, 7.0f, IM_COL32(220, 80, 80, 255));
			DL->AddText(ImVec2(C.x - 20.0f, C.y - 24.0f), IM_COL32(220, 110, 110, 255), "TARGET");

			const int32 N = Count > 0 ? Count : 1;
			for (int32 i = 0; i < N; ++i)
			{
				const float A = -3.14159265f * 0.5f + static_cast<float>(i) / static_cast<float>(N) * 6.2831853f;
				const ImVec2 Q(C.x + cosf(A) * R, C.y + sinf(A) * R);
				const bool bSelf = (i == Slot);
				const ImU32 Col = bSelf
					? (bToken ? IM_COL32(90, 220, 110, 255) : IM_COL32(235, 200, 90, 255))
					: IM_COL32(120, 130, 150, 255);
				DL->AddLine(C, Q, IM_COL32(60, 60, 68, 150), 1.0f);
				DL->AddCircleFilled(Q, bSelf ? 8.0f : 5.5f, Col);
				if (bSelf)
				{
					DL->AddText(ImVec2(Q.x - 8.0f, Q.y + 8.0f), Col, bToken ? "ME*" : "ME");
				}
			}
			if (Slot < 0)
			{
				ImGui::TextDisabled("(not registered as an engager)");
			}
		}
		else
		{
			ImGui::TextDisabled("No blackboard component.");
		}
	}

	// ── 체력 / 체간 시계열 ──────────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Vitality / Posture", ImGuiTreeNodeFlags_DefaultOpen))
	{
		char Overlay[32];
		const float CurHealth = HealthHistory.empty() ? 0.0f : HealthHistory.back();
		std::snprintf(Overlay, sizeof(Overlay), "%.0f%%", CurHealth * 100.0f);
		ImGui::PlotLines("Health", HealthHistory.data(), static_cast<int>(HealthHistory.size()),
			0, Overlay, 0.0f, 1.0f, ImVec2(-1.0f, 56.0f));

		const float CurPosture = PostureHistory.empty() ? 0.0f : PostureHistory.back();
		std::snprintf(Overlay, sizeof(Overlay), "%.0f%%", CurPosture * 100.0f);
		ImGui::PlotLines("Posture", PostureHistory.data(), static_cast<int>(PostureHistory.size()),
			0, Overlay, 0.0f, 1.0f, ImVec2(-1.0f, 56.0f));
	}

	// ── 센서(요약 + 자극 리스트) ────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Perception", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Perception)
		{
			ImGui::Text("Can See Target: %s   (LOS: %s)",
				Perception->CanSeeTarget() ? "YES" : "no",
				Perception->HasLineOfSight() ? "clear" : "blocked");
			ImGui::Text("FOV: %.0f deg   Sight: %.1f   Proximity: %.1f",
				Perception->FieldOfViewDegrees, Perception->SightRange, Perception->ProximityRange);

			static const char* kSenseNames[] = { "Sight", "Proximity", "Damage", "Hearing" };
			const TArray<FAISenseStimulus>& Stimuli = Perception->GetStimuli();
			ImGui::Text("Active stimuli: %d", static_cast<int>(Stimuli.size()));
			for (const FAISenseStimulus& S : Stimuli)
			{
				const int32 Ti = static_cast<int32>(S.Type);
				const char* Tn = (Ti >= 0 && Ti < 4) ? kSenseNames[Ti] : "?";
				ImVec4 Col = ImVec4(0.55f, 0.65f, 0.80f, 1.0f);
				if (S.Type == EAISenseType::Damage)  Col = ImVec4(0.85f, 0.35f, 0.30f, 1.0f);
				if (S.Type == EAISenseType::Hearing) Col = ImVec4(0.45f, 0.80f, 0.80f, 1.0f);
				char Overlay[48];
				std::snprintf(Overlay, sizeof(Overlay), "%-9s str %.2f  age %d", Tn, S.Strength, S.AgeTicks);
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col);
				ImGui::ProgressBar(std::min(1.0f, S.Strength), ImVec2(-1.0f, 0.0f), Overlay);
				ImGui::PopStyleColor();
			}
		}
		else
		{
			ImGui::TextDisabled("No perception component.");
		}
	}

	// ── 전투 프레임 데이터 / 위험공격 / 탄기 ────────────────────────────────────
	if (ImGui::CollapsingHeader("Combat Frame Data", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Move && Move->IsMoveActive())
		{
			const int32 PhaseIdx  = Move->GetPhaseInt();
			const char* PhaseName = (PhaseIdx >= 0 && static_cast<uint32>(PhaseIdx) < GCombatMovePhaseCount) ? GCombatMovePhaseNames[PhaseIdx] : "?";
			const int32 PerilIdx  = Move->GetPerilousTypeInt();
			const char* PerilName = (PerilIdx >= 0 && static_cast<uint32>(PerilIdx) < GPerilousTypeCount) ? GPerilousTypeNames[PerilIdx] : "?";
			ImGui::Text("Phase: %s   frame %d/%d   perilous: %s",
				PhaseName, Move->GetCurrentFrame(), Move->GetTotalFrames(), PerilName);

			const int32 Total = Move->GetTotalFrames();
			const float Frac  = Total > 0 ? static_cast<float>(Move->GetCurrentFrame()) / static_cast<float>(Total) : 0.0f;
			ImVec4 Col;
			switch (static_cast<ECombatMovePhase>(PhaseIdx))
			{
				case ECombatMovePhase::Startup:  Col = ImVec4(0.85f, 0.75f, 0.20f, 1.0f); break;
				case ECombatMovePhase::Active:   Col = ImVec4(0.85f, 0.25f, 0.20f, 1.0f); break;
				case ECombatMovePhase::Recovery: Col = ImVec4(0.25f, 0.55f, 0.85f, 1.0f); break;
				default:                         Col = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); break;
			}
			char Buf[48];
			std::snprintf(Buf, sizeof(Buf), "S%d A%d R%d", Move->GetStartupFrames(), Move->GetActiveFrames(), Move->GetRecoveryFrames());
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col);
			ImGui::ProgressBar(Frac, ImVec2(0.0f, 0.0f), Buf);
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::TextDisabled("No active move.");
		}

		if (Combat)
		{
			const int32 SelfPeril     = Combat->GetActivePerilousTypeInt();
			const char* SelfPerilName = (SelfPeril >= 0 && static_cast<uint32>(SelfPeril) < GPerilousTypeCount) ? GPerilousTypeNames[SelfPeril] : "?";
			const int32 GradeIdx      = Combat->GetLastDeflectGradeInt();
			const char* GradeName     = (GradeIdx >= 0 && static_cast<uint32>(GradeIdx) < GDeflectGradeCount) ? GDeflectGradeNames[GradeIdx] : "?";
			ImGui::Text("Self perilous: %s   Deflecting: %s   Last deflect: %s",
				SelfPerilName, Combat->IsDeflecting() ? "YES" : "no", GradeName);
		}
	}

	// ── 단일 정책 상태(이동·애니·정책 파생) ─────────────────────────────────────
	if (ImGui::CollapsingHeader("Unified State"))
	{
		if (UCharacterStateMachineComponent* SM = Enemy->GetStateMachine())
		{
			static const char* kLocoNames[] = { "Locked", "InputAllowed", "Strafe", "Retreat" };
			const int32 AnimStateId = SM->GetStateInt();
			const int32 LocoIdx = SM->GetLocomotionModeInt();
			const char* LocoName = (LocoIdx >= 0 && LocoIdx < 4) ? kLocoNames[LocoIdx] : "?";
			ImGui::Text("State: %s (anim id %d)   t=%.2fs", SM->GetState().ToString().c_str(), AnimStateId, SM->GetTimeInState());
			ImGui::Text("Locomotion policy: %s   AnimGraph StateId push: %d", LocoName, AnimStateId);
			if (UCharacterMovementComponent* CMove = Enemy->GetCharacterMovement())
			{
				ImGui::Text("Movement speed: %.2f   mode: %s", CMove->GetSpeed(), CMove->IsWalking() ? "Walking" : "Falling");
			}
			AActor* MoveTgt = SM->GetMovementTarget();
			ImGui::TextDisabled("move target: %s", MoveTgt ? MoveTgt->GetName().c_str() : "(none)");
		}
		else
		{
			ImGui::TextDisabled("No state machine component.");
		}
	}

	// ── Blackboard 덤프 ─────────────────────────────────────────────────────────
	if (ImGui::CollapsingHeader("Blackboard"))
	{
		if (BB)
		{
			for (const FBlackboardFloatEntry& Entry : BB->GetFloatEntries())
			{
				ImGui::Text("%-16s %.3f", Entry.Key.ToString().c_str(), Entry.Value);
			}
			for (const FBlackboardBoolEntry& Entry : BB->GetBoolEntries())
			{
				ImGui::Text("%-16s %s", Entry.Key.ToString().c_str(), Entry.Value ? "true" : "false");
			}
			const TArray<FName>& Recent = BB->GetRecentMoves();
			if (!Recent.empty())
			{
				FString Line = "recent: ";
				for (const FName& Mv : Recent)
				{
					Line += Mv.ToString();
					Line += " ";
				}
				ImGui::TextWrapped("%s", Line.c_str());
			}
		}
		else
		{
			ImGui::TextDisabled("No blackboard component.");
		}
	}

	// ── 의사결정 트레이스(시간순 로그) ──────────────────────────────────────────
	if (ImGui::CollapsingHeader("Decision Trace Log"))
	{
		if (Trace)
		{
			ImGui::BeginChild("trace_scroll", ImVec2(0.0f, 160.0f), true);
			for (const FDecisionTraceRecord& R : Trace->GetRecords())
			{
				ImGui::Text("#%lld [%s] -> %s | %s %.2f / %s %.2f / %s %.2f",
					static_cast<long long>(R.Tick),
					R.State.ToString().c_str(),
					R.Chosen.IsValid() ? R.Chosen.ToString().c_str() : "(none)",
					R.TopName[0].ToString().c_str(), R.TopScore[0],
					R.TopName[1].ToString().c_str(), R.TopScore[1],
					R.TopName[2].ToString().c_str(), R.TopScore[2]);
			}
			ImGui::EndChild();
		}
		else
		{
			ImGui::TextDisabled("No decision trace component.");
		}
	}

	ImGui::End();
}
