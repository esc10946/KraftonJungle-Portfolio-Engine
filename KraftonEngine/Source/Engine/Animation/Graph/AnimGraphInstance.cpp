#include "AnimGraphInstance.h"

#include "Animation/Graph/AnimGraphAsset.h"
#include "Animation/Graph/AnimGraphCompiler.h"
#include "Animation/Graph/AnimGraphTypes.h"
#include "Animation/Graph/AnimGraphManager.h"
#include "Animation/Sequence/AnimSequence.h"
#include "Animation/AnimationManager.h"
#include "Animation/Nodes/AnimNode_Base.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Serialization/Archive.h"
#include "Object/GarbageCollection.h"
#include "Object/Object.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
	UAnimSequenceBase* LoadByPath(const FString& Path)
	{
		if (Path.empty() || Path == "None")
		{
			UE_LOG("[AnimDiag][AnimGraph::LoadSequence] skip empty path=%s", Path.c_str());
			return nullptr;
		}
		UAnimSequenceBase* Loaded = FAnimationManager::Get().LoadAnimation(Path);
		if (!IsValid(Loaded))
		{
			Loaded = nullptr;
		}
		if (!Loaded)
		{
			UE_LOG("UAnimGraphInstance: 시퀀스 로드 실패. Path=%s", Path.c_str());
		}
		if (!Loaded)
		{
			UE_LOG("[AnimDiag][AnimGraph::LoadSequence] failed path=%s", Path.c_str());
		}
		else
		{
			UE_LOG("[AnimDiag][AnimGraph::LoadSequence] loaded path=%s object=%s", Path.c_str(), Loaded->GetName().c_str());
		}
		return Loaded;
	}
}

void UAnimGraphInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	UE_LOG(
		"[AnimDiag][AnimGraph::Initialize] begin instance=%s ownerComp=%s graphPath=%s defaultSeq=%s rootMotion=%d graphAsset=%s rootNode=%d",
		GetName().c_str(),
		GetOwningComponent() ? GetOwningComponent()->GetName().c_str() : "None",
		GraphAssetPath.ToString().c_str(),
		DefaultSequencePath.ToString().c_str(),
		static_cast<int>(GetRootMotionMode()),
		IsValid(GraphAsset) ? GraphAsset->GetSourcePath().c_str() : "None",
		GetRootNode() ? 1 : 0);

	USkeletalMesh* Mesh = GetSkeletalMesh();
	if (!Mesh)
	{
		UE_LOG("[AnimDiag][AnimGraph::Initialize] abort: no skeletal mesh instance=%s", GetName().c_str());
		return;
	}
	FSkeletalMesh* MeshAsset = Mesh->GetSkeletalMeshAsset();
	if (!MeshAsset || MeshAsset->Bones.empty())
	{
		UE_LOG(
			"[AnimDiag][AnimGraph::Initialize] abort: invalid mesh asset instance=%s mesh=%s asset=%p bones=%zu",
			GetName().c_str(),
			Mesh->GetAssetPathFileName().c_str(),
			MeshAsset,
			MeshAsset ? MeshAsset->Bones.size() : 0);
		return;
	}

	// GraphAsset resolve order: external SetGraphAsset > GraphAssetPath load > transient single-sequence fallback.
	// A runtime instance with neither GraphAssetPath nor DefaultSequencePath should stay on ref pose; creating
	// an empty SequencePlayer graph here turns missing data into noisy per-instance compile warnings.
	if (GraphAsset && !IsValid(GraphAsset))
	{
		UE_LOG("[AnimDiag][AnimGraph::Initialize] invalid GraphAsset after resolve instance=%s", GetName().c_str());
		GraphAsset = nullptr;
		CompiledAssetVersion = 0;
	}
	const FString GraphPathStr = GraphAssetPath.ToString();
	if (!GraphAsset)
	{
		if (!GraphPathStr.empty() && GraphPathStr != "None")
		{
			UE_LOG("[AnimDiag][AnimGraph::Initialize] loading graph instance=%s path=%s", GetName().c_str(), GraphPathStr.c_str());
			GraphAsset = FAnimGraphManager::Get().Load(GraphPathStr);
			if (!GraphAsset)
			{
				UE_LOG("[AnimDiag][AnimGraph::Initialize] graph load failed instance=%s path=%s", GetName().c_str(), GraphPathStr.c_str());
			}
			if (!GraphAsset)
			{
				UE_LOG("UAnimGraphInstance: AnimGraph 로드 실패 → transient fallback. Path=%s", GraphPathStr.c_str());
			}
		}
	}
	if (!GraphAsset)
	{
		const FString DefaultPathStr = DefaultSequencePath.ToString();
		if (!DefaultPathStr.empty() && DefaultPathStr != "None")
		{
			UE_LOG("[AnimDiag][AnimGraph::Initialize] creating transient default graph instance=%s defaultSeq=%s", GetName().c_str(), DefaultPathStr.c_str());
			GraphAsset = UObjectManager::Get().CreateObject<UAnimGraphAsset>(this);
			if (GraphAsset)
			{
				GraphAsset->InitializeDefault();
			}
		}
		else
		{
			UE_LOG(
				"[AnimDiag][AnimGraph::Initialize] no graph and no default sequence; ref pose instance=%s graphPath=%s defaultSeq=%s",
				GetName().c_str(),
				GraphAssetPath.ToString().c_str(),
				DefaultSequencePath.ToString().c_str());
			RootNode = nullptr;
			OwnedNodes.clear();
			RuntimeVariables.clear();
			CompiledAssetVersion = 0;
			return;
		}
	}

	if (!IsValid(GraphAsset))
	{
		GraphAsset = nullptr;
		RootNode = nullptr;
		OwnedNodes.clear();
		CompiledAssetVersion = 0;
		return;
	}

	// AnimGraph-owned 변수는 자산 기본값에서 per-instance runtime storage 로 초기화한다.
	SyncRuntimeVariablesFromAsset(/*bResetExistingToDefaults*/true);

	// Version 강제 mismatch 로 첫 컴파일 트리거. (자산이 fresh load 라면 Version 이 0 이지만,
	// 다른 인스턴스가 이미 변경해 Version > 0 인 캐시 자산일 수도 있음 — 안전하게 강제.)
	CompiledAssetVersion = GraphAsset->GetVersion() - 1;
	RecompileTreeIfDirty();
	UE_LOG(
		"[AnimDiag][AnimGraph::Initialize] end instance=%s graph=%s version=%u rootNode=%d runtimeVars=%zu",
		GetName().c_str(),
		IsValid(GraphAsset) ? GraphAsset->GetSourcePath().c_str() : "None",
		CompiledAssetVersion,
		GetRootNode() ? 1 : 0,
		RuntimeVariables.size());
}

void UAnimGraphInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// in-editor live preview: 자산이 변경되면 다음 frame UpdateAnimation 의 RootNode->Update
	// 호출 전에 트리 재생성. 새 트리는 즉시 그 frame 부터 평가.
	ResolveGraphAsset();
	RecompileTreeIfDirty();
	static uint32 AnimGraphUpdateFrame = 0;
	if ((++AnimGraphUpdateFrame % 60u) == 0u)
	{
		UE_LOG(
			"[AnimDiag][AnimGraph::Update] sample instance=%s graph=%s version=%u rootNode=%d runtimeVars=%zu dt=%.4f",
			GetName().c_str(),
			IsValid(GraphAsset) ? GraphAsset->GetSourcePath().c_str() : "None",
			CompiledAssetVersion,
			GetRootNode() ? 1 : 0,
			RuntimeVariables.size(),
			DeltaSeconds);
	}
}

bool UAnimGraphInstance::ResolveGraphAsset()
{
	if (GraphAsset && !IsValid(GraphAsset))
	{
		UE_LOG("[AnimDiag][AnimGraph::Resolve] cached graph invalid instance=%s", GetName().c_str());
		GraphAsset = nullptr;
		CompiledAssetVersion = 0;
	}

	if (!GraphAsset)
	{
		const FString GraphPathStr = GraphAssetPath.ToString();
		if (!GraphPathStr.empty() && GraphPathStr != "None")
		{
			UE_LOG("[AnimDiag][AnimGraph::Resolve] loading graph instance=%s path=%s", GetName().c_str(), GraphPathStr.c_str());
			GraphAsset = FAnimGraphManager::Get().Load(GraphPathStr);
			if (!GraphAsset)
			{
				UE_LOG("UAnimGraphInstance: AnimGraph load failed. Path=%s", GraphPathStr.c_str());
				UE_LOG("[AnimDiag][AnimGraph::Resolve] graph load failed instance=%s path=%s", GetName().c_str(), GraphPathStr.c_str());
			}
		}
	}

	if (!GraphAsset)
	{
		const FString DefaultPathStr = DefaultSequencePath.ToString();
		if (!DefaultPathStr.empty() && DefaultPathStr != "None")
		{
			UE_LOG("[AnimDiag][AnimGraph::Resolve] creating transient default graph instance=%s defaultSeq=%s", GetName().c_str(), DefaultPathStr.c_str());
			GraphAsset = UObjectManager::Get().CreateObject<UAnimGraphAsset>(this);
			if (GraphAsset)
			{
				GraphAsset->InitializeDefault();
			}
		}
	}

	if (!IsValid(GraphAsset))
	{
		UE_LOG(
			"[AnimDiag][AnimGraph::Resolve] failed instance=%s graphPath=%s defaultSeq=%s",
			GetName().c_str(),
			GraphAssetPath.ToString().c_str(),
			DefaultSequencePath.ToString().c_str());
		GraphAsset = nullptr;
		RootNode = nullptr;
		OwnedNodes.clear();
		RuntimeVariables.clear();
		CompiledAssetVersion = 0;
		return false;
	}

	return true;
}

void UAnimGraphInstance::RecompileTreeIfDirty()
{
	if (!ResolveGraphAsset())
	{
		UE_LOG("[AnimDiag][AnimGraph::Recompile] skip: resolve failed instance=%s", GetName().c_str());
		return;
	}
	if (GraphAsset->GetVersion() == CompiledAssetVersion) return;

	UE_LOG(
		"[AnimDiag][AnimGraph::Recompile] begin instance=%s graph=%s oldVersion=%u newVersion=%u nodes=%zu defaultSeq=%s",
		GetName().c_str(),
		GraphAsset->GetSourcePath().c_str(),
		CompiledAssetVersion,
		GraphAsset->GetVersion(),
		GraphAsset->GetNodes().size(),
		DefaultSequencePath.ToString().c_str());

	// Recompile 때 새로 추가된 변수는 기본값으로 채우고, 이미 게임 코드가 set 한 값은 유지한다.
	SyncRuntimeVariablesFromAsset(/*bResetExistingToDefaults*/false);

	// 노드별 SequenceRef 재해상 (per-node SequencePath > Instance.DefaultSequencePath > nullptr).
	const FString DefaultPathStr = DefaultSequencePath.ToString();
	for (FAnimGraphNode& Node : const_cast<TArray<FAnimGraphNode>&>(GraphAsset->GetNodes()))
	{
		if (Node.Type != EAnimGraphNodeType::SequencePlayer) continue;

		UE_LOG(
			"[AnimDiag][AnimGraph::Recompile] sequence-player node=%s source=%s default=%s",
			Node.DisplayName.ToString().c_str(),
			Node.SequencePath.c_str(),
			DefaultPathStr.c_str());
		UAnimSequenceBase* Seq = LoadByPath(Node.SequencePath);
		if (!Seq)
		{
			if (!Node.SequencePath.empty() && Node.SequencePath != "None")
			{
				UE_LOG(
					"UAnimGraphInstance: SequencePlayer source failed; trying default sequence. Node=%s Source=%s Default=%s Graph=%s",
					Node.DisplayName.ToString().c_str(),
					Node.SequencePath.c_str(),
					DefaultPathStr.c_str(),
					GraphAssetPath.ToString().c_str());
			}
			Seq = LoadByPath(DefaultPathStr);
		}
		if (!Seq)
		{
			const bool bHadExplicitSequence = !Node.SequencePath.empty() && Node.SequencePath != "None";
			const bool bHadDefaultSequence = !DefaultPathStr.empty() && DefaultPathStr != "None";
			if (bHadExplicitSequence || bHadDefaultSequence)
			{
				UE_LOG(
					"UAnimGraphInstance: SequencePlayer has no valid sequence. Node=%s Source=%s Default=%s Graph=%s",
					Node.DisplayName.ToString().c_str(),
					Node.SequencePath.c_str(),
					DefaultPathStr.c_str(),
					GraphAssetPath.ToString().c_str());
			}
		}
		Node.SequenceRef = IsValid(Seq) ? Seq : nullptr;
		UE_LOG(
			"[AnimDiag][AnimGraph::Recompile] sequence-player result node=%s sequence=%s valid=%d",
			Node.DisplayName.ToString().c_str(),
			Seq ? Seq->GetName().c_str() : "None",
			IsValid(Seq) ? 1 : 0);
	}

	// 기존 트리 폐기 — RootNode 먼저 nullptr 로 끊은 뒤 OwnedNodes clear.
	// (Update 호출 chain 이 다음 줄에 들어가므로 dangling 노출 위험 없음.)
	RootNode = nullptr;
	OwnedNodes.clear();

	FAnimNode_Base* Root = FAnimGraphCompiler::Compile(*GraphAsset, *this);
	if (!Root)
	{
		UE_LOG("[AnimDiag][AnimGraph::Recompile] compile failed instance=%s graph=%s", GetName().c_str(), GraphAsset->GetSourcePath().c_str());
	}
	if (!Root)
	{
		UE_LOG("UAnimGraphInstance: 컴파일 실패 — 트리 미설정, ref pose 유지.");
		CompiledAssetVersion = GraphAsset->GetVersion();
		return;
	}
	SetRootNode(Root);
	CompiledAssetVersion = GraphAsset->GetVersion();
	UE_LOG(
		"[AnimDiag][AnimGraph::Recompile] success instance=%s graph=%s rootNode=%d ownedNodes=%zu runtimeVars=%zu",
		GetName().c_str(),
		GraphAsset->GetSourcePath().c_str(),
		GetRootNode() ? 1 : 0,
		OwnedNodes.size(),
		RuntimeVariables.size());
}

void UAnimGraphInstance::Serialize(FArchive& Ar)
{
	// Editor-set 데모 파라미터만 — 런타임 GraphAsset 포인터는 transient (다음 InitializeAnimation
	// 에서 path 로 재해상). PIE Duplicate (UObject::Duplicate = Serialize 왕복) 가 path 들만 라운드트립.
	// UCharacterAnimInstance 와 동일하게 Super::Serialize 호출 안 함 (ObjectName 직렬화 skip).
	FString SeqPathStr   = Ar.IsSaving() ? DefaultSequencePath.ToString() : FString();
	FString GraphPathStr = Ar.IsSaving() ? GraphAssetPath.ToString()      : FString();
	Ar << SeqPathStr;
	Ar << GraphPathStr;
	if (Ar.IsLoading())
	{
		DefaultSequencePath = FSoftObjectPtr(SeqPathStr);
		GraphAssetPath      = FSoftObjectPtr(GraphPathStr);
	}
}


UAnimGraphAsset* UAnimGraphInstance::GetGraphAsset() const
{
	return IsValid(GraphAsset) ? GraphAsset : nullptr;
}

void UAnimGraphInstance::SetGraphAsset(UAnimGraphAsset* InAsset)
{
	GraphAsset = IsValid(InAsset) ? InAsset : nullptr;
	GraphAssetPath = GraphAsset ? FSoftObjectPtr(GraphAsset->GetSourcePath()) : FSoftObjectPtr("None");
	CompiledAssetVersion = 0;
	RootNode = nullptr;
	OwnedNodes.clear();
	SyncRuntimeVariablesFromAsset(/*bResetExistingToDefaults*/true);
}

void UAnimGraphInstance::PostEditProperty(const char* PropertyName)
{
	Super::PostEditProperty(PropertyName);
	if (!PropertyName) return;

	if (std::strcmp(PropertyName, "GraphAssetPath") == 0 ||
		std::strcmp(PropertyName, "Graph Asset") == 0 ||
		std::strcmp(PropertyName, "DefaultSequencePath") == 0 ||
		std::strcmp(PropertyName, "Default Sequence") == 0)
	{
		GraphAsset = nullptr;
		CompiledAssetVersion = 0;
		RootNode = nullptr;
		OwnedNodes.clear();
		RuntimeVariables.clear();
		UE_LOG(
			"[AnimDiag][AnimGraph::PostEdit] reset graph cache instance=%s property=%s graphPath=%s defaultSeq=%s",
			GetName().c_str(),
			PropertyName,
			GraphAssetPath.ToString().c_str(),
			DefaultSequencePath.ToString().c_str());
	}
}

FAnimGraphRuntimeVariable* UAnimGraphInstance::FindRuntimeVariable(FName VariableName)
{
	if (VariableName == FName::None) return nullptr;
	for (FAnimGraphRuntimeVariable& V : RuntimeVariables)
	{
		if (V.VariableName == VariableName) return &V;
	}
	return nullptr;
}

const FAnimGraphRuntimeVariable* UAnimGraphInstance::FindRuntimeVariable(FName VariableName) const
{
	if (VariableName == FName::None) return nullptr;
	for (const FAnimGraphRuntimeVariable& V : RuntimeVariables)
	{
		if (V.VariableName == VariableName) return &V;
	}
	return nullptr;
}

FAnimGraphRuntimeVariable& UAnimGraphInstance::FindOrAddRuntimeVariable(FName VariableName, EAnimGraphPinType Type)
{
	if (FAnimGraphRuntimeVariable* Existing = FindRuntimeVariable(VariableName))
	{
		Existing->Type = Type;
		return *Existing;
	}

	FAnimGraphRuntimeVariable RuntimeVar;
	RuntimeVar.VariableName = VariableName;
	RuntimeVar.Type = Type;
	RuntimeVariables.push_back(RuntimeVar);
	return RuntimeVariables.back();
}

void UAnimGraphInstance::SyncRuntimeVariablesFromAsset(bool bResetExistingToDefaults)
{
	if (!IsValid(GraphAsset))
	{
		RuntimeVariables.clear();
		return;
	}

	// Keep externally supplied runtime variables. Asset declarations only add/update defaults.
	for (const FAnimGraphVariable& Decl : GraphAsset->GetVariables())
	{
		if (Decl.VariableName == FName::None) continue;
		FAnimGraphRuntimeVariable* Existing = FindRuntimeVariable(Decl.VariableName);
		if (!Existing)
		{
			FAnimGraphRuntimeVariable RuntimeVar;
			RuntimeVar.VariableName = Decl.VariableName;
			RuntimeVar.Type = Decl.Type;
			RuntimeVar.Value = Decl.DefaultValue;
			RuntimeVariables.push_back(RuntimeVar);
		}
		else
		{
			Existing->Type = Decl.Type;
			if (bResetExistingToDefaults)
			{
				Existing->Value = Decl.DefaultValue;
			}
		}
	}
}

bool UAnimGraphInstance::SetGraphVariableFloat(FName VariableName, float Value)
{
	if (!ResolveGraphAsset())
	{
		UE_LOG("[AnimDiag][AnimGraph::SetVarFloat] failed resolve instance=%s var=%s value=%.3f", GetName().c_str(), VariableName.ToString().c_str(), Value);
		return false;
	}
	SyncRuntimeVariablesFromAsset(/*bResetExistingToDefaults*/false);
	FAnimGraphRuntimeVariable& V = FindOrAddRuntimeVariable(VariableName, EAnimGraphPinType::Float);
	V.Value = Value;
	UE_LOG("[AnimDiag][AnimGraph::SetVarFloat] instance=%s var=%s value=%.3f graph=%s", GetName().c_str(), VariableName.ToString().c_str(), Value, IsValid(GraphAsset) ? GraphAsset->GetSourcePath().c_str() : "None");
	return true;
}

bool UAnimGraphInstance::SetGraphVariableBool(FName VariableName, bool bValue)
{
	if (!ResolveGraphAsset())
	{
		UE_LOG("[AnimDiag][AnimGraph::SetVarBool] failed resolve instance=%s var=%s value=%d", GetName().c_str(), VariableName.ToString().c_str(), bValue ? 1 : 0);
		return false;
	}
	SyncRuntimeVariablesFromAsset(/*bResetExistingToDefaults*/false);
	FAnimGraphRuntimeVariable& V = FindOrAddRuntimeVariable(VariableName, EAnimGraphPinType::Bool);
	V.Value = bValue ? 1.0f : 0.0f;
	UE_LOG("[AnimDiag][AnimGraph::SetVarBool] instance=%s var=%s value=%d graph=%s", GetName().c_str(), VariableName.ToString().c_str(), bValue ? 1 : 0, IsValid(GraphAsset) ? GraphAsset->GetSourcePath().c_str() : "None");
	return true;
}

bool UAnimGraphInstance::SetGraphVariableInt(FName VariableName, int32 Value)
{
	if (!ResolveGraphAsset())
	{
		UE_LOG("[AnimDiag][AnimGraph::SetVarInt] failed resolve instance=%s var=%s value=%d", GetName().c_str(), VariableName.ToString().c_str(), Value);
		return false;
	}
	SyncRuntimeVariablesFromAsset(/*bResetExistingToDefaults*/false);
	FAnimGraphRuntimeVariable& V = FindOrAddRuntimeVariable(VariableName, EAnimGraphPinType::Int);
	V.Value = static_cast<float>(Value);
	UE_LOG("[AnimDiag][AnimGraph::SetVarInt] instance=%s var=%s value=%d graph=%s", GetName().c_str(), VariableName.ToString().c_str(), Value, IsValid(GraphAsset) ? GraphAsset->GetSourcePath().c_str() : "None");
	return true;
}

bool UAnimGraphInstance::GetGraphVariableAsFloat(FName VariableName, float& OutValue) const
{
	const FAnimGraphRuntimeVariable* V = FindRuntimeVariable(VariableName);
	if (!V) return false;
	OutValue = V->Value;
	return true;
}

bool UAnimGraphInstance::GetGraphVariableFloat(FName VariableName, float& OutValue) const
{
	return GetGraphVariableAsFloat(VariableName, OutValue);
}

bool UAnimGraphInstance::GetGraphVariableBool(FName VariableName, bool& bOutValue) const
{
	float V = 0.0f;
	if (!GetGraphVariableAsFloat(VariableName, V)) return false;
	bOutValue = V >= 0.5f;
	return true;
}

bool UAnimGraphInstance::GetGraphVariableInt(FName VariableName, int32& OutValue) const
{
	float V = 0.0f;
	if (!GetGraphVariableAsFloat(VariableName, V)) return false;
	OutValue = static_cast<int32>(std::floor(V + 0.5f));
	return true;
}

void UAnimGraphInstance::AddReferencedObjects(FReferenceCollector& Collector)
{
	UAnimInstance::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(GraphAsset, "AnimGraphInstance.GraphAsset");
}

void UAnimGraphInstance::BeginDestroy()
{
	GraphAsset = nullptr;
	CompiledAssetVersion = 0;
	UAnimInstance::BeginDestroy();
}
