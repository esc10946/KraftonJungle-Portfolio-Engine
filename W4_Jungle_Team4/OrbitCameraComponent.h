#pragma once
#include "Component/CameraComponent.h"
#include "Math/Matrix.h"
#include "Math/Utils.h"
#include "Math/Vector.h"

class UOrbitCameraComponent : public UCameraComponent
{
public:
	UOrbitCameraComponent();
	virtual ~UOrbitCameraComponent() = default;

	// 입력 처리
	void OnMouseDrag(float DeltaX, float DeltaY);
	void OnMouseWheel(float Delta);

	// 카메라 초기화
	void SetTarget(const FVector& InTarget);
	void SetRadius(float InRadius);

	// 상태 가져오기
	float GetRadius() const { return Radius; }
	FVector GetTarget() const { return Target; }

private:
	void UpdatePosition(); // 구면 좌표 → 카메라 위치 계산

	float   Radius = 5.0f;   // 오브젝트와의 거리
	float   Yaw = 0.0f;   // 좌우 각도
	float   Pitch = 0.0f;   // 상하 각도
	FVector Target = FVector(0, 0, 0); // 바라보는 오브젝트 위치

	float RotateSensitivity = 0.3f;
	float ZoomSensitivity = 0.5f;
	float PitchMin = -89.0f;
	float PitchMax = 89.0f;
};
