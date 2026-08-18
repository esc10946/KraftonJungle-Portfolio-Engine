#pragma once

#include <fstream>

#include "Serialization/Binary/Archive.h"

// Windows 파일 시스템에 binary bake를 기록하는 writer 구현체입니다.
// SaveToFile이 실제 파일에 쓰기를 원할 때 사용합니다.
class FWindowsBinWriter : public FArchive
{
public:
	explicit FWindowsBinWriter(const char* InPath);

	bool IsLoading() const override;
	bool IsSaving() const override;
	bool IsValid() const override;
	bool Serialize(void* InOutData, size_t InSize) override;
	void Close() override;

private:
	std::ofstream Stream;
};
