#pragma once
#include "Core/CoreTypes.h"

enum class EMemoryCategory : uint8
{
	Heap = 0,
	HeapCount,
	StaticMesh,
	Texture,
	All
};

class EngineStatics
{
private:
	static uint32 NextUUID;

public:
	static uint32 MemoryCategoryBytes[static_cast<uint32>(EMemoryCategory::All)];

	static void AddMemoryUsage(EMemoryCategory Category, uint32 Size)
	{
		if (Category < EMemoryCategory::All)
		{
			MemoryCategoryBytes[static_cast<uint32>(Category)] += Size;
		}
	}

	static void SubtractMemoryUsage(EMemoryCategory Category, uint32 Size)
	{
		if (Category < EMemoryCategory::All)
		{
			MemoryCategoryBytes[static_cast<uint32>(Category)] -= Size;
		}
	}

	static void SetMemoryUsage(EMemoryCategory Category, uint32 Size)
	{
		if (Category < EMemoryCategory::All)
		{
			MemoryCategoryBytes[static_cast<uint32>(Category)] = Size;
		}
	}

	static void OnAllocated(uint32 Size)
	{
		AddMemoryUsage(EMemoryCategory::Heap, Size);
		AddMemoryUsage(EMemoryCategory::HeapCount, 1);
	}

	static void OnDeallocated(uint32 Size)
	{
		SubtractMemoryUsage(EMemoryCategory::Heap, Size);
		SubtractMemoryUsage(EMemoryCategory::HeapCount, 1);
	}

	// Reset UUID generation to a specific value
	static void ResetUUIDGeneration(int Value) { NextUUID = Value; }

	inline static uint32 GetMemoryUsage(EMemoryCategory Category)
	{
		if (Category < EMemoryCategory::All)
		{
			return MemoryCategoryBytes[static_cast<uint32>(Category)];
		}
		return 0;
	}

	static uint32 GenUUID()
	{
		return NextUUID++;
	}
};
