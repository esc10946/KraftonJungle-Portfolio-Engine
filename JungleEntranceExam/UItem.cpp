#include "UItem.h"
#include "ItemRect.h"

UItem::UItem(const FVector& _Location, const FVector& _Direction, const float _Speed, const float _Scale, const FItemDesc& _ItemDesc)
{
	Location = _Location;
	Direction = _Direction;
	Speed = _Speed;
	Scale = _Scale;
	ItemDesc = _ItemDesc;
}

UItem::~UItem()
{
}

// 물리/이동 업데이트
void UItem::Update(float deltaTime)
{
	Location.x += (Speed * deltaTime * Direction.x);
	Location.y += (Speed * deltaTime * Direction.y);

	// TODO: 화면 밖으로 나가면 제거 처리
}

// 렌더링 (상수 버퍼 업데이트)
void UItem::Render(URenderer& renderer)
{
	renderer.UpdateConstant(Location, FVector(Scale, Scale, 0));

	UINT NumVerticesItem = sizeof(item_rect_vertices) / sizeof(FVertexSimple);
	ID3D11Buffer* vertexBufferItem = renderer.CreateVertexBuffer(item_rect_vertices, sizeof(item_rect_vertices));
	renderer.RenderPrimitive(vertexBufferItem, NumVerticesItem);
}

// 벽 충돌 적용
void UItem::ApplyWallCollision()
{
	//Location.x = std::clamp(Location.x, leftBorder + XLength, rightBorder - XLength);
	//// 벽과 충돌 여부를 체크하고 벽에 닿았으면 멈춤
	//if (Location.x < leftBorder + XLength)
	//{
	//	Location.x = leftBorder + XLength;
	//}
	//if (Location.x > rightBorder - XLength)
	//{
	//	Location.x = rightBorder - XLength;
	//}
}

// 충격량 적용
void UItem::ApplyGravity(float deltaTime, const FVector& gravity)
{

}

bool UItem::CheckCollision(const UDiagram* Other)
{
	return false;
}

void UItem::SetScale(const float _Scale)
{
	Scale = _Scale;
}

bool UItem::IsAlive() const
{
	return bAlive;
}

void UItem::Destroy()
{
	bAlive = false;
}

void UItem::ApplyTo(IItemEffectReceiver* Receiver)
{
	if (!bAlive || Receiver == nullptr)
		return;

	ApplyEffect(Receiver);
	bCollected = true;
	Destroy();
}

void UItem::ApplyEffect(IItemEffectReceiver* Receiver)
{
	switch (ItemDesc.Type)
	{
	case EItemType::MultiBall:
		Receiver->SpawnExtraBalls(ItemDesc.IntValue);
		break;

	case EItemType::ScoreBonus:
		Receiver->AddScore(ItemDesc.IntValue);
		break;

	case EItemType::PaddleExpand:
		Receiver->ModifyPaddleSize(ItemDesc.FloatValue, ItemDesc.Duration);
		break;

	case EItemType::PaddleShrink:
		Receiver->ModifyPaddleSize(-ItemDesc.FloatValue, ItemDesc.Duration);
		break;

	case EItemType::BallSpeedUp:
		Receiver->ModifyBallSpeed(ItemDesc.FloatValue, ItemDesc.Duration);
		break;

	case EItemType::BallSpeedDown:
		Receiver->ModifyBallSpeed(ItemDesc.FloatValue, ItemDesc.Duration);
		break;

	default:
		break;
	}
}

//EItemType UItem::GetType() const
//{
//	return ItemDesc.Type;
//}
//
//const FItemDesc& UItem::GetDesc() const
//{
//	return ItemDesc;
//}
