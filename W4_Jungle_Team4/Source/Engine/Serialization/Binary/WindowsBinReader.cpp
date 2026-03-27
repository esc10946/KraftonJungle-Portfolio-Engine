#include "Serialization/Binary/WindowsBinReader.h"

FWindowsBinReader::FWindowsBinReader(const char* InPath)
	: Stream(InPath, std::ios::binary | std::ios::in)
{
}

bool FWindowsBinReader::IsLoading() const
{
	return true;
}

bool FWindowsBinReader::IsSaving() const
{
	return false;
}

bool FWindowsBinReader::IsValid() const
{
	return Stream.good();
}

bool FWindowsBinReader::Serialize(void* InOutData, size_t InSize)
{
	if (!IsValid())
	{
		return false;
	}

	Stream.read(static_cast<char*>(InOutData), static_cast<std::streamsize>(InSize));
	return Stream.good();
}

void FWindowsBinReader::Close()
{
	if (Stream.is_open())
	{
		Stream.close();
	}
}
