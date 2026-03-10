#include "UInputManager.h"

UInputManager* UInputManager::instance = nullptr;

UInputManager* UInputManager::GetInstance()
{
    if (instance == nullptr) {
        instance = new UInputManager();
    }
    return instance;
}

void UInputManager::Update()
{
    for (int i = 0; i < 256; ++i)
    {
        if (GetAsyncKeyState(i) & 0x8000)
        {
            if (KeyStates[i] == EKeyState::None || KeyStates[i] == EKeyState::Up) {
                KeyStates[i] = EKeyState::Down;
            }
            else if (KeyStates[i] == EKeyState::Down) {
                KeyStates[i] = EKeyState::Press;
            }
        }
        else
        {
            if (KeyStates[i] == EKeyState::Down || KeyStates[i] == EKeyState::Press) {
                KeyStates[i] = EKeyState::Up;
            }
            else if (KeyStates[i] == EKeyState::Up) {
                KeyStates[i] = EKeyState::None;
            }
        }
    }
}

void UInputManager::Release()
{
    if (instance != nullptr)
    {
        delete instance;
        instance = nullptr;
    }
}

UInputManager::UInputManager() {
    for (int i = 0; i < 256; ++i) {
        KeyStates[i] = EKeyState::None;
    }
}

UInputManager::~UInputManager()
{
    
}
