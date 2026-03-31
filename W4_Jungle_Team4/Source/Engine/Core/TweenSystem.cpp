#include "TweenSystem.h"

void FTweenSystem::Tick(float DeltaTime)
{
	for (int i = Tweens.size() - 1; i >= 0; --i)
	{
		if (Tweens[i]->Tick(DeltaTime))
		{
			delete Tweens[i];
			Tweens.erase(Tweens.begin() + i);
		}
	}
}