#include "SWidget.h"

void SWidget::Tick(float DeltaTime)
{
	for (SWidget* Child : Children)
	{
		if (Child->bIsVisible)
		{
			Child->Tick(DeltaTime);
		}
	}
}

bool SWidget::IsHovered(FVector2 Coord) const
{
	if (!bIsVisible) return false;
	return Coord.X >= Rect.X && Coord.X <= Rect.X + Rect.Width &&
		   Coord.Y >= Rect.Y && Coord.Y <= Rect.Y + Rect.Height;
}
