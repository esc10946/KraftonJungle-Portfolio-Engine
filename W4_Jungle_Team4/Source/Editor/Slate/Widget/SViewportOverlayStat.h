#pragma once

#include "Core/EngineTypes.h"
#include "Object/EngineStatics.h"

class SViewport;

struct FMemoryStat
{
	FString Name;
	float MemoryBytes;
	FString Format;
	FString PoolType;
};

class SViewportOverlayStat
{
public:
	SViewportOverlayStat(SViewport* InViewport) : Viewport(InViewport) {}

public:
	void DrawFPSStats();
	void DrawMemoryStats();

private:
	SViewport* Viewport = nullptr;

	FMemoryStat MemoryStats[static_cast<uint32>(EMemoryCategory::All)] = {
		{ "Total Allocation Memory", 0, "%.2f MB", "Physical" },
		{ "Total Allocation Count", 0, "%.0f", "Physical" },
		{ "Static Mesh Memory", 0, "%.2f MB", "Physical" },
		{ "Texture Memory", 0, "%.2f MB", "Physical" },
	};
};
