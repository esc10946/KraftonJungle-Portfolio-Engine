#include "AnimGraphManager.h"
#include "Object/GarbageCollection.h"
#include "Object/Object.h"

#include "Animation/Graph/AnimGraphAsset.h"
#include "Asset/AssetPackage.h"
#include "Core/Logging/Log.h"
#include "Platform/Paths.h"
#include "Serialization/WindowsArchive.h"

#include <algorithm>
#include <filesystem>

UAnimGraphAsset* FAnimGraphManager::Load(const FString& Path)
{
	const FString NormalizedPath = FPaths::MakeProjectRelative(Path);
	UE_LOG(
		"[AnimDiag][AnimGraphManager::Load] request path=%s normalized=%s root=%ls",
		Path.c_str(),
		NormalizedPath.c_str(),
		FPaths::RootDir().c_str());

	auto It = LoadedGraphs.find(NormalizedPath);
	if (It != LoadedGraphs.end())
	{
		if (IsValid(It->second))
		{
			UE_LOG("[AnimDiag][AnimGraphManager::Load] cache hit normalized=%s asset=%s", NormalizedPath.c_str(), It->second->GetName().c_str());
			return It->second;
		}
		UE_LOG("[AnimDiag][AnimGraphManager::Load] cache stale normalized=%s", NormalizedPath.c_str());
		LoadedGraphs.erase(It);
	}

	if (!FAssetPackage::IsAssetPackagePath(NormalizedPath))
	{
		UE_LOG("[AnimDiag][AnimGraphManager::Load] invalid package path normalized=%s", NormalizedPath.c_str());
		return nullptr;
	}

	FWindowsBinReader Ar(NormalizedPath);
	if (!Ar.IsValid())
	{
		UE_LOG("[AnimDiag][AnimGraphManager::Load] open failed normalized=%s root=%ls", NormalizedPath.c_str(), FPaths::RootDir().c_str());
		return nullptr;
	}

	FAssetPackageHeader Header;
	FAssetImportMetadata Metadata;
	if (!FAssetPackage::ReadPackagePrelude(Ar, EAssetPackageType::AnimGraph, Header, Metadata))
	{
		UE_LOG("[AnimDiag][AnimGraphManager::Load] prelude failed normalized=%s", NormalizedPath.c_str());
		return nullptr;
	}

	UAnimGraphAsset* NewAsset = UObjectManager::Get().CreateObject<UAnimGraphAsset>();
	NewAsset->Serialize(Ar);

	if (!Ar.IsValid())
	{
		UE_LOG("[AnimDiag][AnimGraphManager::Load] serialize failed normalized=%s asset=%s", NormalizedPath.c_str(), NewAsset ? NewAsset->GetName().c_str() : "None");
		UObjectManager::Get().DestroyObject(NewAsset);
		return nullptr;
	}

	NewAsset->SetSourcePath(NormalizedPath);
	LoadedGraphs.emplace(NormalizedPath, NewAsset);
	UE_LOG(
		"[AnimDiag][AnimGraphManager::Load] loaded normalized=%s asset=%s nodes=%zu vars=%zu version=%u",
		NormalizedPath.c_str(),
		NewAsset->GetName().c_str(),
		NewAsset->GetNodes().size(),
		NewAsset->GetVariables().size(),
		NewAsset->GetVersion());
	return NewAsset;
}

UAnimGraphAsset* FAnimGraphManager::Find(const FString& Path) const
{
	const FString NormalizedPath = FPaths::MakeProjectRelative(Path);
	auto It = LoadedGraphs.find(NormalizedPath);
	if (It == LoadedGraphs.end())
	{
		return nullptr;
	}
	if (IsValid(It->second))
	{
		return It->second;
	}
	return nullptr;
}

bool FAnimGraphManager::Save(UAnimGraphAsset* Asset)
{
	if (!Asset) return false;

	const FString& Path = Asset->GetSourcePath();
	if (Path.empty()) return false;

	FWindowsBinWriter Ar(FPaths::MakeProjectRelative(Path));
	if (!Ar.IsValid()) return false;

	FAssetImportMetadata Metadata;
	if (!FAssetPackage::WritePackagePrelude(Ar, EAssetPackageType::AnimGraph, Metadata))
	{
		return false;
	}

	Asset->Serialize(Ar);
	return Ar.IsValid();
}

void FAnimGraphManager::RefreshAvailableGraphs()
{
	const std::filesystem::path ContentRoot = std::filesystem::path(FPaths::RootDir()) / L"Content";
	if (!std::filesystem::exists(ContentRoot)) return;

	const std::filesystem::path ProjectRoot(FPaths::RootDir());
	AvailableGraphFiles.clear();

	for (const auto& Entry : std::filesystem::recursive_directory_iterator(ContentRoot))
	{
		if (!Entry.is_regular_file()) continue;

		std::wstring Ext = Entry.path().extension().wstring();
		std::transform(Ext.begin(), Ext.end(), Ext.begin(), ::towlower);
		if (Ext != L".uasset") continue;

		const FString RelPath =
			FPaths::ToUtf8(Entry.path().lexically_relative(ProjectRoot).generic_wstring());

		FAssetImportMetadata Metadata;
		if (!FAssetPackage::ReadMetadata(RelPath, EAssetPackageType::AnimGraph, Metadata))
		{
			continue;
		}

		FAssetListItem Item;
		Item.DisplayName = FPaths::ToUtf8(Entry.path().stem().wstring());
		Item.FullPath    = RelPath;
		AvailableGraphFiles.push_back(std::move(Item));
	}
}


void FAnimGraphManager::AddReferencedObjects(FReferenceCollector& Collector)
{
    for (auto& Pair : LoadedGraphs)
    {
        Collector.AddReferencedObject(Pair.second, "FAnimGraphManager.LoadedGraphs");
    }
}

void FAnimGraphManager::ClearCache()
{
    LoadedGraphs.clear();
    AvailableGraphFiles.clear();
}
