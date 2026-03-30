#pragma once

#include "Object/ObjectFactory.h"

class UMesh : public UObject
{
public:
	DECLARE_CLASS(UMesh, UObject)

	UMesh() = default;
	~UMesh() override = default;

	virtual int32 GetNumMaterialSlots() const = 0;
};
