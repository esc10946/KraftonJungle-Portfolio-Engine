#include "UBar.h"
#include "UInputManager.h"

// 생성자 및 소멸자
UBar::UBar(const FVector& _Location, const float _Speed, const float _Scale, int _PlayerNo, EPlaySide _Side)
	: UDiagram()
	, Location(_Location)
	, Speed(_Speed)
	, Scale(_Scale)
	, PlayerNo(_PlayerNo)
	, XLength(1.0f * _Scale)
	, YLength(0.025f)
	, Direction(0)
	, Side(_Side)
{
}

UBar::~UBar()
{
}

// 물리/이동 업데이트
void UBar::Update(float deltaTime)
{
	int leftKey = (PlayerNo == 0) ? VK_LEFT : 'A';
	int rightKey = (PlayerNo == 0) ? VK_RIGHT : 'D';

	UInputManager* input = UInputManager::GetInstance();

	if (!input->GetKeyPress(leftKey) && !input->GetKeyPress(rightKey))
	{
		Direction = 0; // 아무것도 안 누르면 멈춤
	}

	if (input->GetKeyDown(leftKey))
	{
		Direction = -1; // 왼쪽 키 누르면 왼쪽으로
	}
	if (input->GetKeyDown(rightKey))
	{
		Direction = 1;  // 오른쪽 키 누르면 오른쪽으로
	}

	if (input->GetKeyUp(leftKey) && input->GetKeyPress(rightKey))
	{
		Direction = 1;  // 왼쪽 떼고 오른쪽 누르고 있으면 오른쪽으로
	}
	if (input->GetKeyUp(rightKey) && input->GetKeyPress(leftKey))
	{
		Direction = -1; // 오른쪽 떼고 왼쪽 누르고 있으면 왼쪽으로
	}

	Location.x += (Speed * deltaTime * Direction);

	// 벽 충돌 적용
	ApplyWallCollision();
}

// 렌더링 (상수 버퍼 업데이트)
void UBar::Render(URenderer& renderer)
{
	renderer.UpdateConstant(Location, FVector(Scale, 1.0f, 0));
}

// 벽 충돌 적용
void UBar::ApplyWallCollision()
{
	Location.x = std::clamp(Location.x, leftBorder + XLength, rightBorder - XLength);
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
void UBar::ApplyGravity(float deltaTime, const FVector& gravity)
{

}

bool UBar::CheckCollision(const UDiagram* Other)
{
	return false;
}

//void UBar::ResolveCollision(UBar* Other)
//{
//	
//}

void UBar::SetScale(const float _Scale)
{
	Scale = _Scale;
	XLength = 1.000000f * Scale;
}