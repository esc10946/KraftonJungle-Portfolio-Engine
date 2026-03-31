#pragma once

class SViewport;

class SViewportOverlayStat
{
public:
	SViewportOverlayStat(SViewport* InViewport) : Viewport(InViewport) {}

public:
	void DrawFPSStats();
	void DrawMemoryStats();

private:
	SViewport* Viewport = nullptr;
};
