#include "OrbitCameraComponent.h"

DEFINE_CLASS(UOrbitCameraComponent, UCameraComponent)
REGISTER_FACTORY(UOrbitCameraComponent)

void UOrbitCameraComponent::OnOrbit(float DeltaX, float DeltaY)
{
	Yaw += DeltaX * RotateSensitivity;
	Pitch += DeltaY * RotateSensitivity;

	Pitch = Clamp(Pitch, PitchMin, PitchMax);

	UpdatePosition();
}

void UOrbitCameraComponent::OnPan(float DeltaX, float DeltaY)
{
	FVector Right = GetRightVector();
	FVector Up = GetUpVector();

	float PanSpeed = Radius * PanSensitivity;
	Target += Right * DeltaX * PanSpeed;
	Target -= Up * DeltaY * PanSensitivity;

	UpdatePosition();
}

void UOrbitCameraComponent::OnZoom(float Delta)
{
	Radius -= Delta * ZoomSensitivity;

	if (Radius < 1.0f)
		Radius = 1.0f;

	UpdatePosition();
}

void UOrbitCameraComponent::UpdatePosition()
{
	FVector CameraPos;
	CameraPos.X = Target.X + Radius * cos(Pitch) * sin(Yaw);
	CameraPos.Y = Target.Y + Radius * cos(Pitch) * cos(Yaw);
	CameraPos.Z = Target.Z + Radius * sin(Pitch);

	SetWorldLocation(CameraPos);

	LookAt(Target);
}
