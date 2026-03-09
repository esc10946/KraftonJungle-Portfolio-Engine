#include "USceneManager.h"
#include "TitleScene.h"
#include "UGameScene.h"

void USceneManager::LoadScene(ESceneType sceneName)
{
	currentScene->SetActive(false);

	//타이틀 화면 혹은 인게임 화면으로 변환
	if (currentScene->GetSceneType() == sceneName) {
		return;
	}

	currentScene = sceneName == ESceneType::Title ? titleScene : inGameScene;

	currentScene->Init();
	currentScene->SetActive(true);
}

ESceneType USceneManager::GetCurrentSceneName() const
{
	return currentScene->GetSceneType();
}

USceneManager::USceneManager()
{
	titleScene = new UTitleScene();
	inGameScene = new UGameScene();
	currentScene = titleScene;
}

USceneManager::~USceneManager()
{
	titleScene->Release();
	currentScene->Release();

	delete titleScene;
	delete inGameScene;
}
