#include "Editor/UI/Debug/EditorAIDebugWidget.h"

#include "Editor/EditorEngine.h"
#include "Editor/Selection/SelectionManager.h"

#include "ImGui/imgui.h"

#include "Component/AI/AIBlackboardComponent.h"
#include "Component/AI/AIDecisionTraceComponent.h"
#include "Component/AI/AIPerceptionComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/UtilityReasonerComponent.h"
#include "Component/Combat/CombatStateComponent.h"
#include "Component/Combat/CombatMoveComponent.h"
#include "Component/Combat/CombatTypes.h"
#include "Component/Combat/HealthComponent.h"
#include "GameFramework/Pawn/EnemyCharacter.h"
#include "Object/Object.h"
#include "Object/Reflection/UClass.h"

#include <cfloat>
#include <cstdio>

namespace
{
	constexpr int32 kHistoryCapacity = 180;

	void PushRolling(TArray<float>& History, float Value)
	{
		History.push_back(Value);
		while (static_cast<int32>(History.size()) > kHistoryCapacity)
		{
			History.erase(History.begin());
		}
	}
}

void FEditorAIDebugWidget::SampleHistory(AEnemyCharacter* Enemy)
{
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
}

void FEditorAIDebugWidget::Render(float /*DeltaTime*/)
{
	ImGui::SetNextWindowSize(ImVec2(440.0f, 640.0f), ImGuiCond_FirstUseEver);
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
		HealthHistory.clear();
		PostureHistory.clear();
		LastTrackedActor = nullptr;
		ImGui::End();
		return;
	}

	// 추적 대상이 바뀌면 시계열 초기화.
	if (LastTrackedActor != static_cast<const void*>(Enemy))
	{
		HealthHistory.clear();
		PostureHistory.clear();
		LastTrackedActor = static_cast<const void*>(Enemy);
	}
	SampleHistory(Enemy);

	UAIBlackboardComponent*    BB        = Enemy->GetBlackboard();
	UAIPerceptionComponent*    Perception = Enemy->GetPerception();
	UUtilityReasonerComponent* Reasoner  = Enemy->GetReasoner();
	UAIDecisionTraceComponent* Trace     = Enemy->GetDecisionTrace();
	UEnemyAIBrainComponent*    Brain     = Enemy->GetAIBrainComponent();
	UCombatMoveComponent*      Move      = Enemy->GetCombatMove();
	UCombatStateComponent*     Combat    = Enemy->GetCombatStateComponent();

	ImGui::Text("Target: %s", Enemy->GetName().c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("(phase %d)", Enemy->GetAIPhase());
	if (Brain)
	{
		ImGui::Text("HFSM State: %s   (t=%.2fs)", Brain->GetState().ToString().c_str(), Brain->GetStateTime());
	}
	ImGui::Separator();

	// ── 체력 / 체간 시계열 그래프 ──
	if (ImGui::CollapsingHeader("Vitality / Posture", ImGuiTreeNodeFlags_DefaultOpen))
	{
		char Overlay[32];
		const float CurHealth = HealthHistory.empty() ? 0.0f : HealthHistory.back();
		std::snprintf(Overlay, sizeof(Overlay), "%.0f%%", CurHealth * 100.0f);
		ImGui::PlotLines("Health", HealthHistory.data(), static_cast<int>(HealthHistory.size()),
			0, Overlay, 0.0f, 1.0f, ImVec2(0.0f, 60.0f));

		const float CurPosture = PostureHistory.empty() ? 0.0f : PostureHistory.back();
		std::snprintf(Overlay, sizeof(Overlay), "%.0f%%", CurPosture * 100.0f);
		ImGui::PlotLines("Posture", PostureHistory.data(), static_cast<int>(PostureHistory.size()),
			0, Overlay, 0.0f, 1.0f, ImVec2(0.0f, 60.0f));
	}

	// ── 센서 ──
	if (ImGui::CollapsingHeader("Perception", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Perception)
		{
			ImGui::Text("Can See Target: %s", Perception->CanSeeTarget() ? "YES" : "no");
			ImGui::Text("FOV: %.0f deg   Sight: %.1f   Proximity: %.1f",
				Perception->FieldOfViewDegrees, Perception->SightRange, Perception->ProximityRange);
			ImGui::Text("Active stimuli: %d", static_cast<int>(Perception->GetStimuli().size()));
		}
		else
		{
			ImGui::TextDisabled("No perception component.");
		}
	}

	// ── 전투 프레임 데이터 / 위험공격 / 탄기 (Phase 2) ──
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
				case ECombatMovePhase::Startup:  Col = ImVec4(0.85f, 0.75f, 0.20f, 1.0f); break; // 선딜 노랑
				case ECombatMovePhase::Active:   Col = ImVec4(0.85f, 0.25f, 0.20f, 1.0f); break; // 활성 빨강
				case ECombatMovePhase::Recovery: Col = ImVec4(0.25f, 0.55f, 0.85f, 1.0f); break; // 후딜 파랑
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

	// ── Utility 점수 (히스토그램 + 분해 표) ──
	if (ImGui::CollapsingHeader("Utility Reasoner", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (Reasoner)
		{
			const TArray<FUtilityCandidate>& Candidates = Reasoner->GetCandidates();
			if (Candidates.empty())
			{
				ImGui::TextDisabled("No viable candidates this step.");
			}
			else
			{
				TArray<float> Finals;
				Finals.reserve(Candidates.size());
				for (const FUtilityCandidate& Candidate : Candidates)
				{
					Finals.push_back(Candidate.Breakdown.Final);
				}
				ImGui::PlotHistogram("Final score", Finals.data(), static_cast<int>(Finals.size()),
					0, nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 80.0f));

				ImGui::TextDisabled("name           final  | base rng ang thr pos rec rep");
				for (const FUtilityCandidate& C : Candidates)
				{
					const FUtilityScoreBreakdown& B = C.Breakdown;
					ImGui::Text("%s%-13s %6.3f | %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
						C.bChosen ? "*" : " ",
						C.ActionId.ToString().c_str(), B.Final,
						B.Base, B.Range, B.Angle, B.Threat, B.Posture, B.Recovery, B.Repetition);
				}
			}
		}
		else
		{
			ImGui::TextDisabled("No reasoner component.");
		}
	}

	// ── Blackboard ──
	if (ImGui::CollapsingHeader("Blackboard", ImGuiTreeNodeFlags_DefaultOpen))
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
				for (const FName& Move : Recent)
				{
					Line += Move.ToString();
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

	// ── 의사결정 트레이스 (스크롤) ──
	if (ImGui::CollapsingHeader("Decision Trace", ImGuiTreeNodeFlags_DefaultOpen))
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
