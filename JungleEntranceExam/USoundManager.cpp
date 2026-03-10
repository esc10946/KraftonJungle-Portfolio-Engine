#include "USoundManager.h"
// 정적 변수 초기화
USoundManager* USoundManager::instance = nullptr;

void USoundManager::Init()
{
    FMOD::System_Create(&pSystem);

    pSystem->init(32, FMOD_INIT_NORMAL, nullptr);
}

void USoundManager::Update()
{
    if (pSystem != nullptr) {
        pSystem->update();
    }
}

void USoundManager::Release()
{
    // 로드해둔 모든 사운드(CD) 메모리 해제
    for (auto& pair : SoundMap) {
        pair.second->release();
    }
    SoundMap.clear();

    // 시스템 종료
    if (pSystem != nullptr) {
        pSystem->close();
        pSystem->release();
        pSystem = nullptr;
    }
}

void USoundManager::LoadSound(const std::string& name, const std::string& filepath, bool bLoop)
{
    // 이미 같은 이름으로 로드된 소리가 있다면 무시
    if (SoundMap.find(name) != SoundMap.end()) return;

    FMOD::Sound* newSound = nullptr;
    FMOD_MODE mode = bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

    // 파일 경로에서 소리를 읽음
    pSystem->createSound(filepath.c_str(), mode, nullptr, &newSound);

    if (newSound != nullptr) {
        SoundMap[name] = newSound;
    }
}

void USoundManager::Play(const std::string& name)
{
    // Map에서 이름으로 소리를 찾습니다.
    auto iter = SoundMap.find(name);
    if (iter != SoundMap.end())
    {
        pSystem->playSound(iter->second, nullptr, false, nullptr);
    }
}