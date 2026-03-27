#include "Serialization/Binary/WindowsBinWriter.h"

FWindowsBinWriter::FWindowsBinWriter(const char* InPath)
	: Stream(InPath, std::ios::binary | std::ios::out)
{
}

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

bool FWindowsBinWriter::Serialize(void* InOutData, size_t InSize)
{
	if (!IsValid())
	{
		return false;
	}

	Stream.write(static_cast<const char*>(InOutData), static_cast<std::streamsize>(InSize));
	return Stream.good();
}

void FWindowsBinWriter::Close()
{
	if (Stream.is_open())
	{
		Stream.close();
	}
}
