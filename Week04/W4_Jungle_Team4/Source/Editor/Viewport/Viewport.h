#pragma once

#include "Core/EngineTypes.h"
#include "Render/Resource/RenderTarget.h"
#include "Math/Vector.h"

class FViewportClient;
class FD3DDevice;

class FViewport
{
public:
	FViewport(FD3DDevice* Device);
	virtual ~FViewport();

	virtual void Resize(uint32 InLeftTopX, uint32 InLeftTopY, uint32 InWidth, uint32 InHeight);

	void SetViewportClient(FViewportClient* InClient) { Client = InClient; }
	FViewportClient* GetViewportClient() const { return Client; }
	FVector2 GetPos() const { return Pos; }
	FVector2 GetSize() const { return Size; }

	FRenderTarget& GetRenderTarget() { return RenderTarget; }
	FRenderTarget& GetSelectionMaskRenderTarget() { return SelectionMaskRenderTarget; }

protected:
	FD3DDevice* Device = nullptr;
	FRenderTarget RenderTarget;
	FRenderTarget SelectionMaskRenderTarget; // selection mask용 render target

	FViewportClient* Client = nullptr;
	
	FVector2 Pos;
	FVector2 Size;
};
