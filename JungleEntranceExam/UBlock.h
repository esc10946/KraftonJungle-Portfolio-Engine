#pragma once

#include "UDiagram.h"

enum class EBlockType
{
	Normal,
	Hard,
	Immortal
};
enum class EBlockColor
{
	White = 50,
	Orange = 60,
	Cyan = 70,
	Green = 80,
	Red = 90,
	Blue = 100,
	Magenta = 110,
	Yellow = 120,
	Silver = 1,
	Gold = 2
};

enum class EBlockCollision
{
	None = 0,
	Horizontal = 1,
	Vertical = 2,
	Corner = 3,
};

static FColor GetColorFromBlockColor(EBlockColor blockColor)
{
	switch (blockColor)
	{
	case EBlockColor::White:    return FColor(0.9f, 0.9f, 0.9f);
	case EBlockColor::Orange:   return FColor(1.0f, 0.55f, 0.0f);
	case EBlockColor::Cyan:     return FColor(0.0f, 0.9f, 0.9f);
	case EBlockColor::Green:    return FColor(0.0f, 0.8f, 0.0f);
	case EBlockColor::Red:      return FColor(0.9f, 0.0f, 0.0f);
	case EBlockColor::Blue:     return FColor(0.15f, 0.3f, 1.0f);
	case EBlockColor::Magenta:  return FColor(0.9f, 0.0f, 0.9f);
	case EBlockColor::Yellow:   return FColor(1.0f, 0.95f, 0.0f);
	case EBlockColor::Silver:   return FColor(1.00f, 0.55f, 0.10f);
	case EBlockColor::Gold:     return FColor(0.53f, 0.60f, 0.67f);
	default:                    return FColor(1.0f, 1.0f, 1.0f);
	}
}
class URenderer;
class UBlock : public UGameObject
{
public:
	UBlock(EBlockType InType, EBlockColor InColor, int Round=1);
	~UBlock();

	void Init(float cx, float cy, float hw, float hh);

	void Update(float DeltaTime) override;
	void Render(URenderer& Renderer) override;
	bool CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const;// if creating the Ball Class, Make overload function
	int  TakeDamage();

	FColor GetColor() const;
	void ApplyWallCollision() {}

	// 중력 적용
	void ApplyGravity(float deltaTime, const FVector& gravity) {}

	// 다른 도형과 충돌 판정
	bool CheckCollision(const UDiagram* Other) { return false; }

public:
	EBlockType Type;
	EBlockColor Color;
	int MaxHp;
	int CurrHp;
	float CenterX, CenterY;
	float HalfW, HalfH;
	float MinX, MaxX;
	float MinY, MaxY;
	int score;


};
