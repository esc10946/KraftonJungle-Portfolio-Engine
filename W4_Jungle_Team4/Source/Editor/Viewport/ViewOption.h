#pragma once

#include "Core/EngineTypes.h"
#include "Render/Common/ViewTypes.h"

enum class EViewOption : int32
{
	Perspective = 0,
	Top,
	Bottom,
	Left,
	Right,
	Front,
	Back,
	Max
};

struct FViewOption
{
	EViewOption Type = EViewOption::Perspective;
	EViewMode ViewMode = EViewMode::Lit;
	FShowFlags ShowFlags = {};
	bool bIsOrthographic = false;
	float OrthoSize = 10.0f;

	FVector CameraLocation = FVector();
	FVector CameraRotation = FVector();
};
