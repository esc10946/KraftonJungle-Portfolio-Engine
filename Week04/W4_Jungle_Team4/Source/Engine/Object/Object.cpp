#include "Object.h"
#include "EngineStatics.h"

TArray<UObject*> GUObjectArray;

UObject::UObject()
{
	UUID = EngineStatics::GenUUID();
	InternalIndex = static_cast<uint32>(GUObjectArray.size());
	GUObjectArray.push_back(this);
}

UObject::~UObject()
{
	uint32 LastIndex = static_cast<uint32>(GUObjectArray.size() - 1);

	if (InternalIndex != LastIndex)
	{
		UObject* LastObject = GUObjectArray[LastIndex];
		GUObjectArray[InternalIndex] = LastObject;
		LastObject->InternalIndex = InternalIndex;
	}

	GUObjectArray.pop_back();
}

const FTypeInfo UObject::s_TypeInfo = { "UObject", nullptr, sizeof(UObject) };

bool UObject::IsValidLowLevel() const
{
	if (InternalIndex >= GUObjectArray.size())
	{
		return false;
	}

	return GUObjectArray[InternalIndex] == this;
}

void* operator new(size_t Size)
{
	void* Ptr = std::malloc(Size);
	if (Ptr)
	{
		EngineStatics::OnAllocated(static_cast<uint32>(Size));
	}
	return Ptr;
}

void* operator new[](size_t Size)
{
	void* Ptr = std::malloc(Size);
	if (Ptr)
	{
		EngineStatics::OnAllocated(static_cast<uint32>(Size));
	}
	return Ptr;
}

void operator delete(void* Ptr, size_t Size)
{
	if (Ptr)
	{
		EngineStatics::OnDeallocated(static_cast<uint32>(Size));
		std::free(Ptr);
	}
}

void operator delete[](void* Ptr, size_t Size)
{
	if (Ptr)
	{
		EngineStatics::OnDeallocated(static_cast<uint32>(Size));
		std::free(Ptr);
	}
}