#pragma once

#include "AActor.h"

class UTextRenderComponent;

class ASphereActor : public AActor
{
public:
	DECLARE_CLASS(ASphereActor, AActor)
	ASphereActor() = default;

	void InitDefaultComponents();
};

class APlaneActor : public AActor
{
public:
	DECLARE_CLASS(APlaneActor, AActor)
	APlaneActor() = default;

	void InitDefaultComponents();
};

class AStaticMeshActor : public AActor
{
public:
	DECLARE_CLASS(AStaticMeshActor, AActor)
	AStaticMeshActor() = default;

	void InitDefaultComponents();
};

class ACubeActor : public AStaticMeshActor
{
public:
	DECLARE_CLASS(ACubeActor, AStaticMeshActor)
	ACubeActor() = default;

	void InitDefaultComponents();
};

class AAttachTestActor : public AActor
{
public:
	DECLARE_CLASS(AAttachTestActor, AActor)
	AAttachTestActor() = default;

	void InitDefaultComponents();
};
