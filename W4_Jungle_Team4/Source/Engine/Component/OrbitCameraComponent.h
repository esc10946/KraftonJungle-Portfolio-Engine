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
	void OnPan(float DeltaX, float DeltaY); 
	void OnZoom(float Delta);                 

	void SetTarget(const FVector& InTarget) { Target = InTarget; }
	void SetRadius(float InRadius) { Radius = InRadius; }

	float   GetRadius() const { return Radius; }
	FVector GetTarget() const { return Target; }

private:
	void UpdatePosition();

	float   Radius = 5.0f; 
	float   Yaw = 0.0f;
	float   Pitch = 0.0f;
	FVector Target = FVector(0, 0, 0);

	float RotateSensitivity = 0.3f;
	float PanSensitivity = 0.001f;
	float ZoomSensitivity = 0.5f;
	float PitchMin = -89.0f;
	float PitchMax = 89.0f;
};
