#pragma once

#include <functional>

struct ITween
{
	virtual ~ITween() = default;
	virtual bool Tick(float DeltaTime) = 0;
};

template <typename T>
struct FTween : public ITween
{
private:
	using OnUpdateFunc = std::function<void(T)>;

public:
	T StartValue;
	T EndValue;
	float Duration;
	float ElapsedTime;

	OnUpdateFunc UpdateCallback;

	FTween(T InStartValue, T InEndValue, float InDuration)
		: StartValue(InStartValue), EndValue(InEndValue), Duration(InDuration), ElapsedTime(0.0f)
	{
	}

	bool Tick(float DeltaTime) override
	{
		ElapsedTime += DeltaTime;

		if (UpdateCallback)
		{
			UpdateCallback(GetCurrentValue());
		}

		return ElapsedTime >= Duration;
	}
	T GetCurrentValue() const
	{
		if (Duration <= 0.0f) return EndValue; // Avoid division by zero
		float t = ElapsedTime / Duration;
		t = t > 1.0f ? 1.0f : t; // Clamp to [0, 1]
		return StartValue + t * (EndValue - StartValue);
	}

	FTween& SetUpdateCallback(OnUpdateFunc InCallback)
	{
		UpdateCallback = std::move(InCallback);
		return *this;
	}
};