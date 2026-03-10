#pragma once
#include <windows.h>
#include "UGamepadManager.h"
enum class EKeyState
{
    None,   // �� ����
    Down,   // ��� ���� (�� 1�����Ӹ� true)
    Press,  // �� ������ �ִ� ��
    Up      // ��� �� (�� 1�����Ӹ� true)
};

class UInputManager
{

public:
    static UInputManager* GetInstance();

    void Update(); // �� �����Ӹ��� Ű���� ���¸� �������� �Լ�

    bool GetKeyDown(int key) { return KeyStates[key] == EKeyState::Down; }
    bool GetKeyPress(int key) { return KeyStates[key] == EKeyState::Press; }
    bool GetKeyUp(int key) { return KeyStates[key] == EKeyState::Up; }

    void Release();
    float GetAxisX(int playerIndex) const;

private:
    UInputManager();
    ~UInputManager();

    static UInputManager* instance;
    EKeyState KeyStates[256];
    UGamepadManager GamepadMgr;
    float PlayerAxesX[2] = { 0.0f, 0.0f };

};

