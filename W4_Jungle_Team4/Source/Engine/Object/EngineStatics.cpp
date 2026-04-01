#include "EngineStatics.h"

uint32 EngineStatics::NextUUID = 1;

uint32 EngineStatics::MemoryCategoryBytes[static_cast<uint32>(EMemoryCategory::All)] = { 0 };