#pragma once

#include "AActor.h"

class UTextRenderComponent;

class AStaticMeshActor : public AActor
{
public:
	DECLARE_CLASS(AStaticMeshActor, AActor)
	AStaticMeshActor() = default;

	void InitDefaultComponents();
	void UpdateUUIDTextLocation();
};

class ACubeActor : public AStaticMeshActor
{
public:
	DECLARE_CLASS(ACubeActor, AStaticMeshActor)
	ACubeActor() = default;

	void InitDefaultComponents();
};

class ASphereActor : public AStaticMeshActor
{
public:
	DECLARE_CLASS(ASphereActor, AStaticMeshActor)
	ASphereActor() = default;

	void InitDefaultComponents();
};

class APlaneActor : public AStaticMeshActor
{
public:
	DECLARE_CLASS(APlaneActor, AStaticMeshActor)
	APlaneActor() = default;

	void InitDefaultComponents();
};

class AAttachTestActor : public AActor
{
public:
	DECLARE_CLASS(AAttachTestActor, AActor)
	AAttachTestActor() = default;

	void InitDefaultComponents();
};
