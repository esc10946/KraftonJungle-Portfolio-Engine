#include "UGameManager.h"
#include "USceneManager.h"
#include "USoundManager.h"
#include "UItemManager.h"

UGameManager* UGameManager::gameManager = nullptr;

UGameManager::UGameManager()
{
	RessetGM();
}

void UGameManager::initialize()
{
}

//인게임에서 초기화면으로 돌아가는 함수
//SceneManager에게 title을 요청
void UGameManager::Exit()
{
	initialize();

	USoundManager::GetInstance().Play("GameOver");
	USceneManager::GetInstance().LoadScene(ESceneType::Title);
}

UGameManager* UGameManager::GetInstance()
{
	if (gameManager == nullptr) {
		gameManager = new UGameManager();
	}
	return gameManager;
}

void UGameManager::Update(float deltaTime)
{

}

void UGameManager::AddScore(const unsigned int value)
{
	currentScore += value;
	//UpdateScore
}

void UGameManager::SetScore(const unsigned int value)
{
	currentScore = value;
}

void UGameManager::RessetGM()
{
	currentHealth = MaxHealth;
	currentScore = 0;
}

void UGameManager::Release()
{
	if (gameManager != nullptr)
	{
		delete gameManager;
		gameManager = nullptr;
	}
}

void UGameManager::AddHealth(const unsigned int value)
{
	currentHealth += value;
	//최대 체력 
	currentHealth = currentHealth < MaxHealth ? currentHealth : MaxHealth;
}

void UGameManager::SubHealth(const unsigned int value)
{
	// 아이템 관련 리소스 해제
	UItemManager::Get().Clear();

	if (value >= currentHealth) {
		Exit();
		return;
	}
	currentHealth -= value;
}