#include "Serialization/Binary/WindowsBinWriter.h"

// 파일 쓰기 모드 열기
FWindowsBinWriter::FWindowsBinWriter(const char* InPath)
	: Stream(InPath, std::ios::binary | std::ios::out)
{ // std::ios::binary: 텍스트 모드가 아닌 바이너리 그대로 저장
} // std::ios::out: 쓰기 모드

bool FWindowsBinWriter::IsLoading() const
{
	return false;
}

bool FWindowsBinWriter::IsSaving() const
{
	return true;
}

bool FWindowsBinWriter::IsValid() const
{
	return Stream.good();
}

// 메모리 -> 파일로 그대로 복사
bool FWindowsBinWriter::Serialize(void* InOutData, size_t InSize)
{
	if (!IsValid())
	{
		return false;
	}

	// 메모리 주소 InOutData로부터 InSize 바이트를 그대로 파일에 기록
	// 바이너리 저장 -> 타입 정보 없음 -> 읽을 때 같은 구조로 읽어야 함
	// 구조체 그대로 저장 가능
	// 버전 변경 시 깨짐
	Stream.write(static_cast<const char*>(InOutData), static_cast<std::streamsize>(InSize));
	return Stream.good(); // 정상적으로 파일이 열려 있고 쓰기 가능한 상태
}

void FWindowsBinWriter::Close()
{
	if (Stream.is_open())
	{
		Stream.close();
	}
}
