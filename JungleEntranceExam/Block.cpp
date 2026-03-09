#include "Block.h"

#include "UGameObject.h"
#include "Shape.h"
#include <d3d11.h>
#include <cmath>

Block::Block(EBlockType InType, int Round):Type(InType), CenterX(0), CenterY(0), HalfH(0), HalfW(0)
{
	switch (Type)
	{
	case EBlockType::Normal: MaxHp = 1;
	case EBlockType::Hard: MaxHp = 2;
	case EBlockType::Immortal: MaxHp = 10000;
	}
	CurrHp = MaxHp;
	//SetTag("Block");
}

Block::~Block()
{
}

void Block::Init(float cx, float cy, float hw, float hh)
{
}

void Block::Update(float DeltaTime)
{
}

void Block::Render(URenderer& Renderer)
{
}

bool Block::CheckBallCollision(const FVector& BallPos, float Radius, FVector& OutNormal) const
{
	return false;
}

int Block::TakeDamage()
{
	return 0;
}
