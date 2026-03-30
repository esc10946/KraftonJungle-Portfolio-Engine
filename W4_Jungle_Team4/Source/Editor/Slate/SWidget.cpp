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

void SWidget::OnPaint()
{
	for (SWidget* Child : GetChildren())
	{
		if (Child->bIsVisible)
		{
			Child->OnPaint();
		}
	}
}

void SWidget::OnResize(const FRect& NewRect)
{
	Rect = NewRect;
	for (SWidget* Child : GetChildren())
	{
		Child->OnResize(NewRect);
	}
}

bool SWidget::IsHovered(float X, float Y) const
{
	if (!bIsVisible) return false;
	return X >= Rect.X && X <= Rect.X + Rect.Width &&
		   Y >= Rect.Y && Y <= Rect.Y + Rect.Height;
}

json::JSON SWidget::Serialize() const
{
	json::JSON Data;
	Data["WidgetType"] = GetWidgetType();
	Data["Name"] = Name;
	Data["Rect"]["X"] = Rect.X;
	Data["Rect"]["Y"] = Rect.Y;
	Data["Rect"]["Width"] = Rect.Width;
	Data["Rect"]["Height"] = Rect.Height;

	json::JSON ChildrenData = json::Array();
	for (const SWidget* Child : Children)
	{
		ChildrenData.append(Child->Serialize());
	}
	Data["Children"] = ChildrenData;

	return Data;
}

void SWidget::Deserialize(json::JSON& Data)
{
	if (Data.hasKey("Rect"))
	{
		auto RectData = Data["Rect"];
		Rect.X = RectData["X"].ToInt();
		Rect.Y = RectData["Y"].ToInt();
		Rect.Width = RectData["Width"].ToInt();
		Rect.Height = RectData["Height"].ToInt();
	}
	if (Data.hasKey("Name"))
	{
		Name = Data["Name"].ToString();
	}
}
