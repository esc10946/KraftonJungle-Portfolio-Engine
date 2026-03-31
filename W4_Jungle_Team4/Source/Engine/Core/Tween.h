#pragma once

#include <functional>

enum class EEasingType
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut,
};

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
	EEasingType EaseType;

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

		switch (EaseType)
		{
			case EEasingType::Linear:
				break; // t remains unchanged
			case EEasingType::EaseIn:
				t = t * t; // Quadratic ease-in
				break;
			case EEasingType::EaseOut:
				t = t * (2 - t); // Quadratic ease-out
				break;
			case EEasingType::EaseInOut:
				t = t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t; // Quadratic ease-in-out
				break;
		}

		return StartValue + t * (EndValue - StartValue);
	}

	FTween& SetUpdateCallback(OnUpdateFunc InCallback)
	{
		UpdateCallback = std::move(InCallback);
		return *this;
	}

	FTween& SetEasing(EEasingType InEaseType)
	{
		EaseType = InEaseType;
		return *this;
	}
};