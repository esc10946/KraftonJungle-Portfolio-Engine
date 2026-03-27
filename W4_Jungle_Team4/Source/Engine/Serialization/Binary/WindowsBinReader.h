#pragma once

#include <fstream>

#include "Serialization/Binary/Archive.h"

// Windows 파일 시스템에서 binary bake를 읽는 reader 구현체입니다.
// LoadFromFile이 실제 bake 파일을 복원할 때 사용합니다.
class FWindowsBinReader : public FArchive
{
public:
	explicit FWindowsBinReader(const char* InPath);

	bool IsLoading() const override;
	bool IsSaving() const override;
	bool IsValid() const override;
	bool Serialize(void* InOutData, size_t InSize) override;
	void Close() override;

private:
	std::ifstream Stream;
};
