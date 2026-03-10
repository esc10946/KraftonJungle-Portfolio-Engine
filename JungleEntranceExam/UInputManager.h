#pragma once
#include <windows.h>

enum class EKeyState
{
    None,   // 안 눌림
    Down,   // 방금 눌림 (딱 1프레임만 true)
    Press,  // 꾹 누르고 있는 중
    Up      // 방금 뗌 (딱 1프레임만 true)
};

class UInputManager
{
public:
    static UInputManager* GetInstance();

    void Update(); // 매 프레임마다 키보드 상태를 갱신해줄 함수

    bool GetKeyDown(int key) { return KeyStates[key] == EKeyState::Down; }
    bool GetKeyPress(int key) { return KeyStates[key] == EKeyState::Press; }
    bool GetKeyUp(int key) { return KeyStates[key] == EKeyState::Up; }

    void Release();

private:
    UInputManager();
    ~UInputManager();

    static UInputManager* instance;

    EKeyState KeyStates[256];
};

