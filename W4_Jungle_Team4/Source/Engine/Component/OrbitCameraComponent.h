#pragma once
#include "Component/CameraComponent.h"
#include "Math/Matrix.h"
#include "Math/Utils.h"
#include "Math/Vector.h"

class UOrbitCameraComponent : public UCameraComponent
{
public:
	DECLARE_CLASS(UOrbitCameraComponent, UCameraComponent)

	UOrbitCameraComponent() = default;
	virtual ~UOrbitCameraComponent() = default;

	void OnOrbit(float DeltaX, float DeltaY); 
	void OnPan(float DeltaX, float DeltaY, float ModelRadius);
	void OnZoom(float Delta);                 

	void SetTarget(const FVector& InTarget) { Target = InTarget; }
	void SetRadius(float InRadius) { Radius = InRadius; }

	float   GetRadius() const { return Radius; }
	FVector GetTarget() const { return Target; }

	void UpdatePosition();

	float RotateSensitivity = 0.1f;
	float PanSensitivity = 0.01f;
	float ZoomSensitivity = 0.5f;

private:
	float   Radius = 5.0f; 
	float   Yaw = 0.0f;
	float   Pitch = 0.0f;
	FVector Target = FVector(0, 0, 0);

	float PitchMin = -89.0f;
	float PitchMax = 89.0f;
};