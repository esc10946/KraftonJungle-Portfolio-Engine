#include "UGamepadManager.h"
extern "C" {

}
UGamepadManager::UGamepadManager()
{
}

UGamepadManager::~UGamepadManager()
{
	if (pGamepad)
	{
		pGamepad->Release();
		pGamepad = nullptr;
	}
}

void UGamepadManager::Update()
{




}

bool UGamepadManager::IsPressed(GamepadButtons btn) const
{
	return false;
}

float UGamepadManager::GetLeftThumbstickX() const
{
	return 0.0f;
}
