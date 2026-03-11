#include "UGameManager.h"
#include "USceneManager.h"
#include "USoundManager.h"
#include "UItemManager.h"
#include "FileInfo.h"
UGameManager* UGameManager::gameManager = nullptr;

UGameManager::UGameManager()
{
	RessetGM();
}

void UGameManager::initialize()
{
}

//?멸쾶?꾩뿉??珥덇린?붾㈃?쇰줈 ?뚯븘媛???⑥닔
//SceneManager?먭쾶 title???붿껌
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
	//理쒕? 泥대젰 
	currentHealth = currentHealth < MaxHealth ? currentHealth : MaxHealth;
}

void UGameManager::SubHealth(const unsigned int value)
{
	// ?꾩씠??愿??由ъ냼???댁젣
	UItemManager::Get().Clear();

	if (value >= currentHealth) {
		HightScoreUpdate(currentScore);
		Exit();
		return;
	}
	currentHealth -= value;
}
