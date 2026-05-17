#include "EditorViewer.h"
#include "EditorEngine.h"
#include "Editor/EditorRenderPipeline.h"
#include "Editor/Selection/SelectionManager.h"
#include "Component/GizmoComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/TransformProxy.h"
#include "Animation/AnimInstance.h"
#include "Asset/AnimationSequence.h"
#include "Asset/SkeletonAsset.h"
#include "Core/AssetPathPolicy.h"
#include "Core/Logging/Log.h"
#include "Core/Paths.h"
#include "Core/ResourceManager.h"
#include "Engine/Geometry/Ray.h"
#include "GameFramework/PrimitiveActors.h"
#include "Math/Vector4.h"
#include "Object/Object.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cwctype>
#include <filesystem>

#include "Component/PostProcess/Light/AmbientLightComponent.h"

namespace
{
static int32 GetSkeletalMeshBoneCount(const USkeletalMesh* Mesh)
{
    return Mesh ? static_cast<int32>(Mesh->GetBones().size()) : 0;
}

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

    return ToLowerNormalizedPath(FPaths::ToUtf8(MeshPath.parent_path().generic_wstring()))
        == ToLowerNormalizedPath(FPaths::ToUtf8(AnimPath.parent_path().generic_wstring()))
        && GetImportedFbxStem(Mesh->GetAssetPathFileName()) == GetImportedFbxStem(AnimationPath);
}

float DistanceSquaredRaySegment(const FRay& Ray, const FVector& SegmentStart, const FVector& SegmentEnd, float& OutRayT)
{
    const FVector RayDir = Ray.Direction.GetSafeNormal();
    const FVector SegmentDir = SegmentEnd - SegmentStart;
    const FVector RayToSegment = Ray.Origin - SegmentStart;

    constexpr float Epsilon = 1.0e-6f;
    const float RayLenSq = std::max(FVector::DotProduct(RayDir, RayDir), Epsilon);
    const float SegmentLenSq = FVector::DotProduct(SegmentDir, SegmentDir);

    float RayT = 0.0f;
    float SegmentT = 0.0f;

    if (SegmentLenSq <= Epsilon)
    {
        RayT = std::max(0.0f, -FVector::DotProduct(RayDir, RayToSegment) / RayLenSq);
    }
    else
    {
        const float RaySegmentDot = FVector::DotProduct(RayDir, SegmentDir);
        const float RayOriginDot = FVector::DotProduct(RayDir, RayToSegment);
        const float SegmentOriginDot = FVector::DotProduct(SegmentDir, RayToSegment);
        const float Denom = RayLenSq * SegmentLenSq - RaySegmentDot * RaySegmentDot;

        if (std::abs(Denom) > Epsilon)
        {
            RayT = std::max(0.0f, (RaySegmentDot * SegmentOriginDot - RayOriginDot * SegmentLenSq) / Denom);
        }

        SegmentT = (RaySegmentDot * RayT + SegmentOriginDot) / SegmentLenSq;
        if (SegmentT < 0.0f)
        {
            SegmentT = 0.0f;
            RayT = std::max(0.0f, -RayOriginDot / RayLenSq);
        }
        else if (SegmentT > 1.0f)
        {
            SegmentT = 1.0f;
            RayT = std::max(0.0f, (RaySegmentDot - RayOriginDot) / RayLenSq);
        }
    }

    const FVector ClosestOnRay = Ray.Origin + RayDir * RayT;
    const FVector ClosestOnSegment = SegmentStart + SegmentDir * SegmentT;
    OutRayT = RayT;
    return FVector::DistSquared(ClosestOnRay, ClosestOnSegment);
}

float DistanceSquaredPointSegment2D(
    float PointX,
    float PointY,
    float SegmentStartX,
    float SegmentStartY,
    float SegmentEndX,
    float SegmentEndY)
{
    const float SegmentX = SegmentEndX - SegmentStartX;
    const float SegmentY = SegmentEndY - SegmentStartY;
    const float SegmentLenSq = SegmentX * SegmentX + SegmentY * SegmentY;
    if (SegmentLenSq <= 1.0e-6f)
    {
        const float DX = PointX - SegmentStartX;
        const float DY = PointY - SegmentStartY;
        return DX * DX + DY * DY;
    }

    const float T = std::clamp(
        ((PointX - SegmentStartX) * SegmentX + (PointY - SegmentStartY) * SegmentY) / SegmentLenSq,
        0.0f,
        1.0f);
    const float ClosestX = SegmentStartX + SegmentX * T;
    const float ClosestY = SegmentStartY + SegmentY * T;
    const float DX = PointX - ClosestX;
    const float DY = PointY - ClosestY;
    return DX * DX + DY * DY;
}

bool ProjectWorldToViewport(
    const FViewportCamera& Camera,
    const FVector& WorldPos,
    float ViewportWidth,
    float ViewportHeight,
    float& OutViewportX,
    float& OutViewportY,
    float& OutDepth)
{
    const FVector4 Clip = FMatrix::Identity.TransformVector4(FVector4(WorldPos, 1.0f), Camera.GetViewProjectionMatrix());
    if (MathUtil::IsNearlyZero(Clip.W))
    {
        return false;
    }

    const float InvW = 1.0f / Clip.W;
    const float NdcX = Clip.X * InvW;
    const float NdcY = Clip.Y * InvW;
    const float NdcZ = Clip.Z * InvW;
    if (NdcX < -1.0f || NdcX > 1.0f || NdcY < -1.0f || NdcY > 1.0f)
    {
        return false;
    }

    OutViewportX = (NdcX * 0.5f + 0.5f) * ViewportWidth;
    OutViewportY = (1.0f - (NdcY * 0.5f + 0.5f)) * ViewportHeight;
    OutDepth = NdcZ;
    return true;
}
}

void FEditorViewer::Init(
    FWindowsWindow* InWindow,
    UEditorEngine* InEditor,
    UWorld* InWorld,
    FSelectionManager* InSelectionManager)
{
    Viewport.SetClient(&Client);

    Client.Initialize(InWindow, InEditor);
    Client.SetWorld(InWorld);
    Client.SetGizmo(InSelectionManager->GetGizmo());
    Client.SetSelectionManager(InSelectionManager);
    Client.SetSceneEditingShortcutsEnabled(false);

    Client.SetViewport(&Viewport);
    Client.SetState(&Viewport.GetState());
    Client.SetBonePickHandler([this](float LocalX, float LocalY)
    {
        return HandleBonePick(LocalX, LocalY);
    });

    Client.SetViewportType(EEditorViewportType::EVT_Perspective);
    Client.CreateCamera();
    Client.ApplyCameraMode();
    Viewport.GetState().ViewMode = EViewMode::Lit_BlinnPhong;
    Viewport.GetState().LightCullMode = ELightCullMode::None;

	FViewportRect Rect = { 0, 0, 300, 300 };
    Viewport.SetRect(Rect);

    ViewTarget = InWorld->SpawnActor<ASkeletalMeshActor>();
    ViewTarget->InitDefaultComponents();
    ViewTarget->SetFName(FName("ViewerActor"));
    ViewTarget->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

    ADirectionalLightActor* DirectionalLight = InWorld->SpawnActor<ADirectionalLightActor>();
    if (DirectionalLight)
    {
        DirectionalLight->InitDefaultComponents();
        DirectionalLight->SetFName(FName("Viewer Directional Light"));
        DirectionalLight->SetActorLocation(FVector(100000.0f, 100000.0f, 100000.0f));
        DirectionalLight->SetActorRotation(FVector(0.0f, 44.0f, 0.0f));
    }

    AAmbientLightActor* AmbientLight = InWorld->SpawnActor<AAmbientLightActor>();
    if (AmbientLight)
    {
        AmbientLight->InitDefaultComponents();
        AmbientLight->SetFName(FName("Viewer Ambient Light"));
        AmbientLight->SetActorLocation(FVector(100000.0f, 100000.0f, 100000.0f));
    }
    
    AmbientLight->FindComponent<UAmbientLightComponent>()->Intensity = 0.7f;

    InWorld->SyncSpatialIndex();
}

void FEditorViewer::Shutdown()
{
    // UEditorEngine::Shutdown 흐름:
    //   CloseScene() (ViewTarget actor 포함 destroy) → ... → Viewer.Shutdown()
    // 이 시점에 ViewTarget raw 포인터는 dangling이므로 RemoveComponent를 호출하면 안 됨.
    // SocketPreview 컴포넌트들은 ViewTarget actor 소멸자가 이미 함께 destroy함.
    // 우리는 단순히 map만 비우고 포인터를 null로 잡는다.
    SocketPreviewMeshes.clear();
    if (PreviewAnimInstance)
    {
        UObjectManager::Get().DestroyObject(PreviewAnimInstance);
        PreviewAnimInstance = nullptr;
    }
    CurrentAnimationSequence = nullptr;
    AnimationSequencePath.clear();
    ViewTarget = nullptr;
    Client.SetBonePickHandler(nullptr);

    Viewport.SetClient(nullptr);
}

void FEditorViewer::Tick(float DeltaTime)
{
    Client.Tick(DeltaTime);
}

void FEditorViewer::SelectBone(int32 BoneIndex)
{
    if (!ViewTarget)
    {
        return;
    }

    USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent();
    USkeletalMesh* Mesh = SkelComp ? SkelComp->GetSkeletalMesh() : nullptr;
    if (!SkelComp || !Mesh || BoneIndex < 0 || BoneIndex >= static_cast<int32>(Mesh->GetBones().size()))
    {
        return;
    }

    SelectedBoneIndex = BoneIndex;
    SelectedSocketIndex = -1;

    if (FSelectionManager* SelectionManager = Client.GetSelectionManager())
    {
        SelectionManager->Select(ViewTarget);
    }

    if (UGizmoComponent* Gizmo = Client.GetGizmo())
    {
        Gizmo->SetProxy(std::make_shared<FBoneTransformProxy>(SkelComp, SelectedBoneIndex));
        Gizmo->SetSelectedActors(nullptr);

        FMatrix LocalTransform = SkelComp->GetBoneLocalTransform(SelectedBoneIndex);
        FVector DummyLocation;
        FVector DummyScale;
        FMatrix RotationMatrix;
        LocalTransform.Decompose(DummyLocation, RotationMatrix, DummyScale);
        CachedRotation = RotationMatrix.GetEuler();
    }
}

void FEditorViewer::SelectSocket(int32 SocketIndex)
{
    if (!ViewTarget)
    {
        return;
    }

    USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent();
    USkeletalMesh* Mesh = SkelComp ? SkelComp->GetSkeletalMesh() : nullptr;
    FSkeletalMesh* MeshData = Mesh ? Mesh->GetMeshData() : nullptr;
    if (!SkelComp || !MeshData || SocketIndex < 0 || SocketIndex >= static_cast<int32>(MeshData->Sockets.size()))
    {
        return;
    }

    const FName SocketName = MeshData->Sockets[SocketIndex].Name;
    SelectedBoneIndex = -1;
    SelectedSocketIndex = SocketIndex;

    if (FSelectionManager* SelectionManager = Client.GetSelectionManager())
    {
        SelectionManager->Select(ViewTarget);
    }

    if (UGizmoComponent* Gizmo = Client.GetGizmo())
    {
        Gizmo->SetProxy(std::make_shared<FSocketTransformProxy>(SkelComp, SocketName));
        Gizmo->SetSelectedActors(nullptr);
    }
}

void FEditorViewer::ClearSelection()
{
    SelectedBoneIndex = -1;
    SelectedSocketIndex = -1;
    CachedRotation = FVector::ZeroVector;

    if (FSelectionManager* SelectionManager = Client.GetSelectionManager())
    {
        SelectionManager->ClearSelection();
        return;
    }

    if (UGizmoComponent* Gizmo = Client.GetGizmo())
    {
        Gizmo->Deactivate();
    }
}

void FEditorViewer::NotifySocketDeleted(int32 SocketIndex)
{
    if (SelectedSocketIndex == SocketIndex)
    {
        ClearSelection();
    }
    else if (SelectedSocketIndex > SocketIndex)
    {
        --SelectedSocketIndex;
    }
}

bool FEditorViewer::HandleBonePick(float LocalX, float LocalY)
{
    int32 PickedBoneIndex = -1;
    if (!TryPickBone(LocalX, LocalY, PickedBoneIndex))
    {
        ClearSelection();
        return true;
    }

    SelectBone(PickedBoneIndex);
    return true;
}

bool FEditorViewer::TryPickBone(float LocalX, float LocalY, int32& OutBoneIndex) const
{
    OutBoneIndex = -1;
    if (!ViewTarget || !Client.GetCamera())
    {
        return false;
    }

    USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent();
    USkeletalMesh* Mesh = SkelComp ? SkelComp->GetSkeletalMesh() : nullptr;
    if (!SkelComp || !Mesh || !Mesh->HasValidMeshData())
    {
        return false;
    }

    const FViewportRect& Rect = Viewport.GetRect();
    if (Rect.Width <= 0 || Rect.Height <= 0)
    {
        return false;
    }

    SkelComp->EnsureSkinningUpdated();

    const FViewportCamera& Camera = *Client.GetCamera();
    const float ViewportWidth = static_cast<float>(Rect.Width);
    const float ViewportHeight = static_cast<float>(Rect.Height);

    const TArray<FBoneInfo>& Bones = Mesh->GetBones();
    float BestScore = FLT_MAX;
    int32 BestBoneIndex = -1;

    constexpr float PickRadiusPixels = 12.0f;
    constexpr float PickRadiusPixelsSq = PickRadiusPixels * PickRadiusPixels;

    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Bones.size()); ++BoneIndex)
    {
        const int32 ParentIndex = Bones[BoneIndex].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= static_cast<int32>(Bones.size()))
        {
            continue;
        }

        const FVector SegmentStart = SkelComp->GetBoneWorldMatrix(ParentIndex).GetTranslation();
        const FVector SegmentEnd = SkelComp->GetBoneWorldMatrix(BoneIndex).GetTranslation();
        const float SegmentLength = FVector::Dist(SegmentStart, SegmentEnd);
        if (SegmentLength <= 1.0e-4f)
        {
            continue;
        }

        float StartX = 0.0f;
        float StartY = 0.0f;
        float StartDepth = 0.0f;
        float EndX = 0.0f;
        float EndY = 0.0f;
        float EndDepth = 0.0f;
        if (!ProjectWorldToViewport(Camera, SegmentStart, ViewportWidth, ViewportHeight, StartX, StartY, StartDepth) ||
            !ProjectWorldToViewport(Camera, SegmentEnd, ViewportWidth, ViewportHeight, EndX, EndY, EndDepth))
        {
            continue;
        }

        const float DistanceSq = DistanceSquaredPointSegment2D(LocalX, LocalY, StartX, StartY, EndX, EndY);
        if (DistanceSq > PickRadiusPixelsSq)
        {
            continue;
        }

        const float DepthScore = std::min(StartDepth, EndDepth);
        const float Score = DistanceSq + DepthScore * 0.001f;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestBoneIndex = BoneIndex;
        }
    }

    if (BestBoneIndex >= 0)
    {
        OutBoneIndex = BestBoneIndex;
        return true;
    }

    const FRay Ray = Camera.DeprojectScreenToWorld(LocalX, LocalY, ViewportWidth, ViewportHeight);
    BestScore = FLT_MAX;

    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(Bones.size()); ++BoneIndex)
    {
        const int32 ParentIndex = Bones[BoneIndex].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= static_cast<int32>(Bones.size()))
        {
            continue;
        }

        const FVector SegmentStart = SkelComp->GetBoneWorldMatrix(ParentIndex).GetTranslation();
        const FVector SegmentEnd = SkelComp->GetBoneWorldMatrix(BoneIndex).GetTranslation();
        const float SegmentLength = FVector::Dist(SegmentStart, SegmentEnd);
        if (SegmentLength <= 1.0e-4f)
        {
            continue;
        }

        float RayT = 0.0f;
        const float DistanceSq = DistanceSquaredRaySegment(Ray, SegmentStart, SegmentEnd, RayT);
        const float PickRadius = std::clamp(SegmentLength * 0.18f, 1.0f, 12.0f);
        if (DistanceSq > PickRadius * PickRadius)
        {
            continue;
        }

        const float Score = DistanceSq + RayT * 0.0001f;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestBoneIndex = BoneIndex;
        }
    }

    if (BestBoneIndex < 0)
    {
        return false;
    }

    OutBoneIndex = BestBoneIndex;
    return true;
}

void FEditorViewer::SetSocketPreviewMesh(const FName& SocketName, const FString& StaticMeshPath)
{
    if (!ViewTarget) return;

    // 같은 socket에 이미 preview가 있으면 먼저 제거 (덮어쓰기 동작)
    ClearSocketPreview(SocketName);

    UStaticMesh* Mesh = FResourceManager::Get().LoadStaticMesh(StaticMeshPath);
    if (!Mesh) return;

    UStaticMeshComponent* Preview = ViewTarget->AddComponent<UStaticMeshComponent>();
    if (!Preview) return;

    // 휘발성 + 에디터 전용. Scene 저장에 안 들어가고, 게임 빌드 render에 안 잡힘.
    Preview->SetTransient(true);
    Preview->SetEditorOnly(true);
    Preview->SetStaticMesh(Mesh);

    if (USkeletalMeshComponent* SkComp = ViewTarget->GetSkeletalMeshComponent())
    {
        Preview->AttachToComponent(SkComp, SocketName);
    }

    SocketPreviewMeshes[SocketName] = Preview;
}

void FEditorViewer::ClearSocketPreview(const FName& SocketName)
{
    auto It = SocketPreviewMeshes.find(SocketName);
    if (It == SocketPreviewMeshes.end()) return;

    if (ViewTarget && It->second)
    {
        ViewTarget->RemoveComponent(It->second);
        // RemoveComponent가 UObjectManager::DestroyObject까지 호출 — 추가 cleanup 불필요.
    }
    SocketPreviewMeshes.erase(It);
}

void FEditorViewer::ClearAllSocketPreviews()
{
    if (ViewTarget)
    {
        for (auto& [Name, Comp] : SocketPreviewMeshes)
        {
            if (Comp)
            {
                ViewTarget->RemoveComponent(Comp);
            }
        }
    }
    SocketPreviewMeshes.clear();
}

UStaticMeshComponent* FEditorViewer::FindPreviewMesh(const FName& SocketName) const
{
    auto It = SocketPreviewMeshes.find(SocketName);
    return It != SocketPreviewMeshes.end() ? It->second : nullptr;
}

void FEditorViewer::ChangeTarget(const FString& InFileName)
{
    FileName = InFileName;
    USkeletalMesh* SkelMesh = FResourceManager::Get().LoadSkeletalMesh(InFileName.c_str());
    if (SkelMesh)
    {
	    ViewTarget->GetSkeletalMeshComponent()->SetSkeletalMesh(SkelMesh);
    }

    UE_LOG(
        "[EditorViewer] ChangeTarget | Requested=%s | LoadedMesh=%s | Bones=%d",
        InFileName.c_str(),
        GetSkeletalMeshPath(SkelMesh),
        GetSkeletalMeshBoneCount(SkelMesh));

    ClearAnimationSequence();
}

bool FEditorViewer::IsAnimationSequenceCompatible(const FString& AnimationPath) const
{
    if (!ViewTarget)
    {
        return false;
    }

    USkeletalMeshComponent* SkelMeshComp = ViewTarget->GetSkeletalMeshComponent();
    USkeletalMesh* SkelMesh = nullptr;
    if (SkelMeshComp)
    {
        SkelMesh = SkelMeshComp->GetSkeletalMesh();
    }

    if (!SkelMesh)
    {
        return false;
    }

    UAnimationSequence* Sequence = FResourceManager::Get().LoadAnimationSequence(AnimationPath);
    return IsAnimationCompatibleWithMesh(SkelMesh, Sequence, AnimationPath);
}

bool FEditorViewer::SetAnimationSequence(const FString& AnimationPath)
{
    if (!ViewTarget)
    {
        return false;
    }

    USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent();
    if (!SkelComp || !SkelComp->GetSkeletalMesh())
    {
        return false;
    }

    if (!FileName.empty())
    {
        if (USkeletalMesh* TargetMesh = FResourceManager::Get().LoadSkeletalMesh(FileName))
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
            "[EditorViewer] RejectAnimationSequence | ViewerMesh=%s | MeshSkeleton=%s | Animation=%s | AnimationSkeleton=%s",
            GetSkeletalMeshPath(SkelComp->GetSkeletalMesh()),
            ResolveSkeletonPathFromMesh(SkelComp->GetSkeletalMesh()).c_str(),
            AnimationPath.c_str(),
            AnimationSkeletonPath.c_str());
        return false;
    }

    if (!PreviewAnimInstance)
    {
        PreviewAnimInstance = UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>();
        PreviewAnimInstance->SetLooping(true);
    }

    CurrentAnimationSequence = Sequence;
    AnimationSequencePath = AnimationPath;

    SkelComp->SetAnimInstance(PreviewAnimInstance);
    PreviewAnimInstance->SetSequence(Sequence);
    PreviewAnimInstance->SetLooping(true);
    PreviewAnimInstance->SetCurrentTime(0.0f);
    return true;
}

void FEditorViewer::ClearAnimationSequence()
{
    CurrentAnimationSequence = nullptr;
    AnimationSequencePath.clear();

    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->Stop();
        PreviewAnimInstance->SetSequence(nullptr);
    }

    if (ViewTarget)
    {
        if (USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent())
        {
            SkelComp->SetAnimInstance(nullptr);
            SkelComp->ResetToBindPose();
        }
    }
}

void FEditorViewer::PlayAnimation()
{
    if (PreviewAnimInstance && CurrentAnimationSequence)
    {
        PreviewAnimInstance->Play();
    }
}

void FEditorViewer::PauseAnimation()
{
    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->Pause();
    }
}

void FEditorViewer::StopAnimation()
{
    if (PreviewAnimInstance)
    {
        PreviewAnimInstance->Stop();
    }

    if (ViewTarget)
    {
        if (USkeletalMeshComponent* SkelComp = ViewTarget->GetSkeletalMeshComponent())
        {
            SkelComp->ResetToBindPose();
        }
    }
}

void FEditorViewer::SetAnimationTime(float Time)
{
    if (PreviewAnimInstance && CurrentAnimationSequence)
    {
        PreviewAnimInstance->SetCurrentTime(Time);
    }
}

float FEditorViewer::GetAnimationTime() const
{
    return PreviewAnimInstance ? PreviewAnimInstance->GetCurrentTime() : 0.0f;
}

float FEditorViewer::GetAnimationLength() const
{
    return CurrentAnimationSequence ? CurrentAnimationSequence->GetPlayLength() : 0.0f;
}

bool FEditorViewer::IsAnimationPlaying() const
{
    return PreviewAnimInstance && PreviewAnimInstance->IsPlaying();
}

bool FEditorViewer::IsAnimationPaused() const
{
    return PreviewAnimInstance && PreviewAnimInstance->IsPaused();
}
