#include "AnimationViewer.h"
#include "Animation/AnimInstance.h"
#include "Asset/AnimationSequence.h"
#include "Asset/SkeletonAsset.h"
#include "Component/SkeletalMeshComponent.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "GameFramework/PrimitiveActors.h"
#include "Object/Object.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>

namespace
{
static const char* GetSkeletalMeshPath(const USkeletalMesh* Mesh)
{
    return Mesh ? Mesh->GetAssetPathFileName().c_str() : "<None>";
}

static FString ToLowerNormalizedPath(const FString& Path)
{
    std::wstring Wide = FPaths::ToWide(FPaths::Normalize(Path));
    std::transform(
        Wide.begin(),
        Wide.end(),
        Wide.begin(),
        [](wchar_t Ch)
        {
            return static_cast<wchar_t>(std::towlower(Ch));
        });
    return FPaths::ToUtf8(Wide);
}

static FString ResolveSkeletonPathFromMesh(const USkeletalMesh* Mesh)
{
    if (!Mesh)
    {
        return FString();
    }

    if (const USkeletonAsset* SkeletonAsset = Mesh->GetSkeletonAsset())
    {
        const FString& SkeletonPath = SkeletonAsset->GetAssetPathFileName();
        if (!SkeletonPath.empty())
        {
            return ToLowerNormalizedPath(SkeletonPath);
        }
    }

    if (!Mesh->GetSkeletonSourcePath().empty())
    {
        return ToLowerNormalizedPath(FAssetPathPolicy::ResolveAssetRelativePath(
            Mesh->GetAssetPathFileName(),
            Mesh->GetSkeletonSourcePath()));
    }

    return FString();
}

static FString ResolveSkeletonPathFromAnimation(const UAnimationSequence* Sequence)
{
    const FAnimationSequence* SequenceData = nullptr;
    if (Sequence)
    {
        SequenceData = Sequence->GetSequenceData();
    }

    if (!SequenceData || SequenceData->SkeletonSourcePath.empty())
    {
        return FString();
    }

    return ToLowerNormalizedPath(FAssetPathPolicy::ResolveAssetRelativePath(
        Sequence->GetAssetPathFileName(),
        SequenceData->SkeletonSourcePath));
}

static FString GetImportedFbxStem(const FString& AssetPath)
{
    std::filesystem::path FsPath(FPaths::ToWide(FPaths::Normalize(AssetPath)));
    std::wstring Stem = FsPath.stem().wstring();
    std::transform(Stem.begin(), Stem.end(), Stem.begin(), ::towlower);

    static const std::wstring Markers[] = { L"_skeletalmesh_", L"_skeleton_", L"_anim_" };
    for (const std::wstring& Marker : Markers)
    {
        const size_t MarkerPos = Stem.find(Marker);
        if (MarkerPos != std::wstring::npos)
        {
            Stem = Stem.substr(0, MarkerPos);
            break;
        }
    }

    return FPaths::ToUtf8(Stem);
}

static bool IsAnimationCompatibleWithMesh(const USkeletalMesh* Mesh, const UAnimationSequence* Sequence, const FString& AnimationPath)
{
    if (!Mesh || !Sequence)
    {
        return false;
    }

    const FString MeshSkeleton = ResolveSkeletonPathFromMesh(Mesh);
    const FString AnimationSkeleton = ResolveSkeletonPathFromAnimation(Sequence);
    if (!MeshSkeleton.empty() && !AnimationSkeleton.empty())
    {
        return MeshSkeleton == AnimationSkeleton;
    }

    const std::filesystem::path MeshPath(FPaths::ToWide(FPaths::Normalize(Mesh->GetAssetPathFileName())));
    const std::filesystem::path AnimPath(FPaths::ToWide(FPaths::Normalize(AnimationPath)));

    return ToLowerNormalizedPath(FPaths::ToUtf8(MeshPath.parent_path().generic_wstring())) ==
               ToLowerNormalizedPath(FPaths::ToUtf8(AnimPath.parent_path().generic_wstring())) &&
           GetImportedFbxStem(Mesh->GetAssetPathFileName()) == GetImportedFbxStem(AnimationPath);
}
}

FAnimationViewer::~FAnimationViewer()
{
    Shutdown();
}

void FAnimationViewer::Shutdown()
{
    FEditorViewer::Shutdown();
    if (PreviewAnimInstance)
    {
        UObjectManager::Get().DestroyObject(PreviewAnimInstance);
        PreviewAnimInstance = nullptr;
    }
    CurrentAnimationSequence = nullptr;
    AnimationSequencePath.clear();
}

void FAnimationViewer::ChangeTarget(const FString& InFileName)
{
    FEditorViewer::ChangeTarget(InFileName);
    ClearAnimationSequence();
}

bool FAnimationViewer::IsAnimationSequenceCompatible(const FString& AnimationPath) const
{
    ASkeletalMeshActor* ViewTarget = GetViewTarget();
    if (!ViewTarget)
    {
        return false;
    }

    USkeletalMeshComponent* SkelMeshComp = ViewTarget->GetSkeletalMeshComponent();
    USkeletalMesh* SkelMesh = SkelMeshComp ? SkelMeshComp->GetSkeletalMesh() : nullptr;
    if (!SkelMesh)
    {
        return false;
    }

    UAnimationSequence* Sequence = FResourceManager::Get().LoadAnimationSequence(AnimationPath);
    return IsAnimationCompatibleWithMesh(SkelMesh, Sequence, AnimationPath);
}

bool FAnimationViewer::SetAnimationSequence(const FString& AnimationPath)
{
    ASkeletalMeshActor* ViewTarget = GetViewTarget();
    if (!ViewTarget)
    {
        return false;
    }

    USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent();
    if (!SkelComp || !SkelComp->GetSkeletalMesh())
    {
        return false;
    }

    if (!GetFileName().empty())
    {
        if (USkeletalMesh* TargetMesh = FResourceManager::Get().LoadSkeletalMesh(GetFileName()))
        {
            SkelComp->SetSkeletalMesh(TargetMesh);
        }
    }

    UAnimationSequence* Sequence = FResourceManager::Get().LoadAnimationSequence(AnimationPath);
    if (!Sequence)
    {
        return false;
    }

    if (!IsAnimationCompatibleWithMesh(SkelComp->GetSkeletalMesh(), Sequence, AnimationPath))
    {
        const FAnimationSequence* SequenceData = Sequence->GetSequenceData();
        FString AnimationSkeletonPath = "<None>";
        if (SequenceData)
        {
            AnimationSkeletonPath = ResolveSkeletonPathFromAnimation(Sequence);
        }

        UE_LOG_WARNING(
            "[AnimationViewer] RejectAnimationSequence | ViewerMesh=%s | MeshSkeleton=%s | Animation=%s | AnimationSkeleton=%s",
            GetSkeletalMeshPath(SkelComp->GetSkeletalMesh()),
            ResolveSkeletonPathFromMesh(SkelComp->GetSkeletalMesh()).c_str(),
            AnimationPath.c_str(),
            AnimationSkeletonPath.c_str());
        return false;
    }

    if (!PreviewAnimInstance)
    {
        PreviewAnimInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
        PreviewAnimInstance->SetLooping(bLooping);
        PreviewAnimInstance->SetPlayRate(PlayRate);
    }

    CurrentAnimationSequence = Sequence;
    AnimationSequencePath = AnimationPath;

    SkelComp->SetAnimInstance(PreviewAnimInstance);
    PreviewAnimInstance->SetSequence(Sequence);
    PreviewAnimInstance->SetLooping(bLooping);
    PreviewAnimInstance->SetPlayRate(PlayRate);
    PreviewAnimInstance->SetCurrentTime(0.0f);
    return true;
}

void FAnimationViewer::ClearAnimationSequence()
{
    CurrentAnimationSequence = nullptr;
    AnimationSequencePath.clear();

    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->Stop();
        PreviewAnimInstance->SetSequence(nullptr);
    }

    if (ASkeletalMeshActor* ViewTarget = GetViewTarget())
    {
        if (USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent())
        {
            SkelComp->SetAnimInstance(nullptr);
            SkelComp->ResetToBindPose();
        }
    }
}

void FAnimationViewer::PlayAnimation()
{
    if (PreviewAnimInstance && CurrentAnimationSequence)
    {
        PreviewAnimInstance->Play();
    }
}

void FAnimationViewer::PauseAnimation()
{
    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->Pause();
    }
}

void FAnimationViewer::StopAnimation()
{
    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->Stop();
    }

    if (ASkeletalMeshActor* ViewTarget = GetViewTarget())
    {
        if (USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent())
        {
            SkelComp->ResetToBindPose();
        }
    }
}

void FAnimationViewer::SetLooping(bool bInLooping)
{
    bLooping = bInLooping;
    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->SetLooping(bLooping);
    }
}

bool FAnimationViewer::IsLooping() const
{
    return PreviewAnimInstance ? PreviewAnimInstance->IsLooping() : bLooping;
}

void FAnimationViewer::SetPlayRate(float InPlayRate)
{
    PlayRate = std::max(0.001f, InPlayRate);
    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->SetPlayRate(PlayRate);
    }
}

float FAnimationViewer::GetPlayRate() const
{
    return PreviewAnimInstance ? PreviewAnimInstance->GetPlayRate() : PlayRate;
}

void FAnimationViewer::SetAnimationTime(float Time)
{
    if (PreviewAnimInstance && CurrentAnimationSequence)
    {
        PreviewAnimInstance->SetCurrentTime(Time);
    }
}

float FAnimationViewer::GetAnimationTime() const
{
    return PreviewAnimInstance ? PreviewAnimInstance->GetCurrentTime() : 0.0f;
}

float FAnimationViewer::GetAnimationLength() const
{
    return CurrentAnimationSequence ? CurrentAnimationSequence->GetPlayLength() : 0.0f;
}

bool FAnimationViewer::IsAnimationPlaying() const
{
    return PreviewAnimInstance && PreviewAnimInstance->IsPlaying();
}

bool FAnimationViewer::IsAnimationPaused() const
{
    return PreviewAnimInstance && PreviewAnimInstance->IsPaused();
}
