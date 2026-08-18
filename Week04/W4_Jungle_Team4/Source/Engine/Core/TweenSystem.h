#pragma once

#include "Tween.h"
#include "Core/Singleton.h"
#include "EngineTypes.h"

class FTweenSystem : public TSingleton<FTweenSystem>
{
	friend class TSingleton<FTweenSystem>;
public:
	FTweenSystem() = default;
	~FTweenSystem() = default;

public:
	void Tick(float DeltaTime);

	template <typename T>
	FTween<T>& Create(T StartValue, T EndValue, float Duration)
	{
		FTween<T>* NewTween = new FTween<T>(StartValue, EndValue, Duration);
		Tweens.push_back(NewTween);
		return *NewTween;
	}

private:
	TArray<ITween*> Tweens;
};