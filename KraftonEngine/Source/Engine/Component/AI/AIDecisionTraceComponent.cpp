#include "Component/AI/AIDecisionTraceComponent.h"

#include "Component/AI/AIPerceptionComponent.h"
#include "Component/AI/EnemyAIBrainComponent.h"
#include "Component/AI/UtilityReasonerComponent.h"
#include "GameFramework/AActor.h"

#include <algorithm>

void UAIDecisionTraceComponent::ClearRecords()
{
    Records.clear();
}

void UAIDecisionTraceComponent::RecordDecision()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FDecisionTraceRecord Record;
    Record.Tick = NextTick++;

    if (UEnemyAIBrainComponent* Brain = Owner->GetComponentByClass<UEnemyAIBrainComponent>())
    {
        Record.State = Brain->GetState();
    }

    if (UUtilityReasonerComponent* Reasoner = Owner->GetComponentByClass<UUtilityReasonerComponent>())
    {
        const TArray<FUtilityCandidate>& Candidates = Reasoner->GetCandidates();
        Record.CandidateCount = static_cast<int32>(Candidates.size());
        Record.Chosen         = Reasoner->GetChosenAction();

        // Final 점수 내림차순 top-3.
        TArray<int32> Order;
        Order.reserve(Candidates.size());
        for (int32 Index = 0; Index < static_cast<int32>(Candidates.size()); ++Index)
        {
            Order.push_back(Index);
        }
        std::sort(Order.begin(), Order.end(), [&Candidates](int32 A, int32 B)
        {
            return Candidates[A].Breakdown.Final > Candidates[B].Breakdown.Final;
        });
        for (int32 Slot = 0; Slot < 3 && Slot < static_cast<int32>(Order.size()); ++Slot)
        {
            const FUtilityCandidate& Candidate = Candidates[Order[Slot]];
            Record.TopName[Slot]  = Candidate.ActionId;
            Record.TopScore[Slot] = Candidate.Breakdown.Final;
        }
    }

    if (UAIPerceptionComponent* Perception = Owner->GetComponentByClass<UAIPerceptionComponent>())
    {
        Record.StimulusCount = static_cast<int32>(Perception->GetStimuli().size());
    }

    Records.insert(Records.begin(), Record);
    const int32 Limit = MaxRecords > 0 ? MaxRecords : 0;
    while (static_cast<int32>(Records.size()) > Limit)
    {
        Records.pop_back();
    }
}
