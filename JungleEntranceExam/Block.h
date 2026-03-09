#pragma once
#include "UGameObject.h"
struct FVector;

enum class EBlockType
{
	Normal,
	Hard,
	Immortal
};
class Block : public UGameObject
{
public:
	Block(EBlockType InType, int Round = 1);
	~Block();

	void Init(float cx, float cy, float hw, float hh);

	void Update(float DeltaTime) override;
	void Render(URenderer& Renderer) override;
	bool CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const;
	int  TakeDamage();

public:
	EBlockType Type;
	int MaxHp;
	int CurrHp;
	float CenterX, CenterY;
	float HalfW, HalfH;
	//static int TotalScore;


};
