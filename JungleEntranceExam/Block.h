#pragma once
#include "UGameObject.h"
struct FVector;

enum class EBlockType
{
	Normal,
	Hard,
	Immortal
};
enum class EBlcokColor
{
	White = 50,
	Orange = 60,
	Cyan = 70,
	Green = 80,
	Red = 90,
	Blue = 100,
	Magenta = 110,
	Yellow = 120,
	Silver = 50
};
class Block : public UGameObject
{
public:
	Block(EBlockType InType, int Round = 1);
	~Block();

	void Init(float cx, float cy, float hw, float hh);

	void Update(float DeltaTime) override;
	void Render(URenderer& Renderer) override;
	bool CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const;// if creating the Ball Class, Make overload function
	int  TakeDamage();

public:
	EBlockType Type;
	EBlcokColor Color;
	int MaxHp;
	int CurrHp;
	float CenterX, CenterY;
	float HalfW, HalfH;



};
