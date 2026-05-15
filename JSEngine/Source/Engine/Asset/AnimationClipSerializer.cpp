#include "Asset/AnimationClipSerializer.h"

#include "Core/Paths.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>

namespace
{
constexpr uint32 ANIMATION_CLIP_BINARY_MAGIC = 0x4D494E41; // 'ANIM'
constexpr uint32 ANIMATION_CLIP_BINARY_VERSION = 2;
constexpr uint32 MAX_ANIMATION_BONE_TRACK_COUNT = 65'536;
constexpr uint32 MAX_ANIMATION_SHAPE_KEY_TRACK_COUNT = 1'000'000;
constexpr uint32 MAX_ANIMATION_CURVE_COUNT = 2'000'000;
constexpr uint32 MAX_ANIMATION_KEY_COUNT = 20'000'000;
constexpr uint32 MAX_ANIMATION_STRING_LENGTH = 16'384;

static uint64 GetFileWriteTimeTicks(const FString& Path)
{
    const FString NormalizedPath = FPaths::Normalize(Path);
    const std::filesystem::path FilePath(FPaths::ToAbsolute(FPaths::ToWide(NormalizedPath)));
    if (!std::filesystem::exists(FilePath))
    {
        return 0;
    }

    const auto WriteTime = std::filesystem::last_write_time(FilePath);
    const auto Duration = WriteTime.time_since_epoch();
    return static_cast<uint64>(std::chrono::duration_cast<std::chrono::seconds>(Duration).count());
}

static void CountClipData(const FAnimationClip& Clip, uint32& OutCurveCount, uint32& OutKeyCount)
{
    OutCurveCount = 0;
    OutKeyCount = 0;

    auto CountCurve = [&OutCurveCount, &OutKeyCount](const FAnimationFloatCurve& Curve)
    {
        ++OutCurveCount;
        OutKeyCount += static_cast<uint32>(Curve.Keys.size());
    };

    for (const FBoneAnimationTrack& Track : Clip.BoneTracks)
    {
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            CountCurve(Track.Translation[Axis]);
            CountCurve(Track.Rotation[Axis]);
            CountCurve(Track.Scale[Axis]);
        }
    }

    for (const FShapeKeyAnimationTrack& Track : Clip.ShapeKeyTracks)
    {
        CountCurve(Track.WeightCurve);
    }
}

static bool IsValidHeader(const FAnimationClipBinaryHeader& Header)
{
    return Header.MagicNumber == ANIMATION_CLIP_BINARY_MAGIC
        && Header.Version == ANIMATION_CLIP_BINARY_VERSION
        && Header.BoneTrackCount <= MAX_ANIMATION_BONE_TRACK_COUNT
        && Header.ShapeKeyTrackCount <= MAX_ANIMATION_SHAPE_KEY_TRACK_COUNT
        && Header.CurveCount <= MAX_ANIMATION_CURVE_COUNT
        && Header.KeyCount <= MAX_ANIMATION_KEY_COUNT;
}
}

void FAnimationClipSerializer::WriteBool(std::ofstream& Out, bool Value)
{
    const uint8 Byte = Value ? 1 : 0;
    Out.write(reinterpret_cast<const char*>(&Byte), sizeof(Byte));
}

void FAnimationClipSerializer::WriteInt32LE(std::ofstream& Out, int32 Value)
{
    uint32 U = static_cast<uint32>(Value);
    WriteUInt32LE(Out, U);
}

void FAnimationClipSerializer::WriteUInt32LE(std::ofstream& Out, uint32 Value)
{
    uint8 Bytes[4] = {
        static_cast<uint8>(Value & 0xFF),
        static_cast<uint8>((Value >> 8) & 0xFF),
        static_cast<uint8>((Value >> 16) & 0xFF),
        static_cast<uint8>((Value >> 24) & 0xFF),
    };
    Out.write(reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
}

void FAnimationClipSerializer::WriteUInt64LE(std::ofstream& Out, uint64 Value)
{
    for (int32 i = 0; i < 8; ++i)
    {
        const uint8 Byte = static_cast<uint8>((Value >> (i * 8)) & 0xFF);
        Out.write(reinterpret_cast<const char*>(&Byte), sizeof(Byte));
    }
}

void FAnimationClipSerializer::WriteFloatLE(std::ofstream& Out, float Value)
{
    uint32 Bits = 0;
    std::memcpy(&Bits, &Value, sizeof(float));
    WriteUInt32LE(Out, Bits);
}

void FAnimationClipSerializer::WriteString(std::ofstream& Out, const FString& Value)
{
    const uint32 Length = static_cast<uint32>(Value.size());
    WriteUInt32LE(Out, Length);
    if (Length > 0)
    {
        Out.write(Value.data(), Length);
    }
}

void FAnimationClipSerializer::WriteVector(std::ofstream& Out, const FVector& Value)
{
    WriteFloatLE(Out, Value.X);
    WriteFloatLE(Out, Value.Y);
    WriteFloatLE(Out, Value.Z);
}

void FAnimationClipSerializer::WriteHeader(std::ofstream& Out, const FAnimationClipBinaryHeader& Header)
{
    WriteUInt32LE(Out, Header.MagicNumber);
    WriteUInt32LE(Out, Header.Version);
    WriteUInt32LE(Out, Header.BoneTrackCount);
    WriteUInt32LE(Out, Header.ShapeKeyTrackCount);
    WriteUInt32LE(Out, Header.CurveCount);
    WriteUInt32LE(Out, Header.KeyCount);
    WriteUInt64LE(Out, Header.SourceFileWriteTime);
}

void FAnimationClipSerializer::WriteCurveKey(std::ofstream& Out, const FAnimationCurveKey& Key)
{
    WriteFloatLE(Out, Key.TimeSeconds);
    WriteFloatLE(Out, Key.Value);
}

void FAnimationClipSerializer::WriteFloatCurve(std::ofstream& Out, const FAnimationFloatCurve& Curve)
{
    WriteInt32LE(Out, static_cast<int32>(Curve.Channel));
    WriteFloatLE(Out, Curve.DefaultValue);
    WriteBool(Out, Curve.bHasCurve);
    WriteUInt32LE(Out, static_cast<uint32>(Curve.Keys.size()));
    for (const FAnimationCurveKey& Key : Curve.Keys)
    {
        WriteCurveKey(Out, Key);
    }
}

void FAnimationClipSerializer::WriteBoneTrack(std::ofstream& Out, const FBoneAnimationTrack& Track)
{
    WriteString(Out, Track.BoneName);
    WriteVector(Out, Track.DefaultTranslation);
    WriteVector(Out, Track.DefaultRotationEuler);
    WriteVector(Out, Track.DefaultScale);

    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        WriteFloatCurve(Out, Track.Translation[Axis]);
        WriteFloatCurve(Out, Track.Rotation[Axis]);
        WriteFloatCurve(Out, Track.Scale[Axis]);
    }
}

void FAnimationClipSerializer::WriteShapeKeyTrack(std::ofstream& Out, const FShapeKeyAnimationTrack& Track)
{
    WriteString(Out, Track.MeshNodeName);
    WriteString(Out, Track.ShapeKeyName);
    WriteInt32LE(Out, Track.ShapeKeyIndex);
    WriteFloatCurve(Out, Track.WeightCurve);
}

bool FAnimationClipSerializer::ReadBool(std::ifstream& In, bool& OutValue) const
{
    uint8 Byte = 0;
    In.read(reinterpret_cast<char*>(&Byte), sizeof(Byte));
    if (!In.good())
    {
        return false;
    }
    OutValue = Byte != 0;
    return true;
}

bool FAnimationClipSerializer::ReadInt32LE(std::ifstream& In, int32& OutValue) const
{
    uint32 U = 0;
    if (!ReadUInt32LE(In, U))
    {
        return false;
    }
    OutValue = static_cast<int32>(U);
    return true;
}

bool FAnimationClipSerializer::ReadUInt32LE(std::ifstream& In, uint32& OutValue) const
{
    uint8 Bytes[4] = {};
    In.read(reinterpret_cast<char*>(Bytes), sizeof(Bytes));
    if (!In.good())
    {
        return false;
    }

    OutValue =
        static_cast<uint32>(Bytes[0]) |
        (static_cast<uint32>(Bytes[1]) << 8) |
        (static_cast<uint32>(Bytes[2]) << 16) |
        (static_cast<uint32>(Bytes[3]) << 24);
    return true;
}

bool FAnimationClipSerializer::ReadUInt64LE(std::ifstream& In, uint64& OutValue) const
{
    OutValue = 0;
    for (int32 i = 0; i < 8; ++i)
    {
        uint8 Byte = 0;
        In.read(reinterpret_cast<char*>(&Byte), sizeof(Byte));
        if (!In.good())
        {
            return false;
        }
        OutValue |= static_cast<uint64>(Byte) << (i * 8);
    }
    return true;
}

bool FAnimationClipSerializer::ReadFloatLE(std::ifstream& In, float& OutValue) const
{
    uint32 Bits = 0;
    if (!ReadUInt32LE(In, Bits))
    {
        return false;
    }
    std::memcpy(&OutValue, &Bits, sizeof(float));
    return true;
}

bool FAnimationClipSerializer::ReadString(std::ifstream& In, FString& OutValue) const
{
    uint32 Length = 0;
    if (!ReadUInt32LE(In, Length) || Length > MAX_ANIMATION_STRING_LENGTH)
    {
        return false;
    }

    OutValue.resize(Length);
    if (Length > 0)
    {
        In.read(OutValue.data(), Length);
        if (!In.good())
        {
            return false;
        }
    }

    return true;
}

bool FAnimationClipSerializer::ReadVector(std::ifstream& In, FVector& OutValue) const
{
    return ReadFloatLE(In, OutValue.X)
        && ReadFloatLE(In, OutValue.Y)
        && ReadFloatLE(In, OutValue.Z);
}

bool FAnimationClipSerializer::ReadHeader(std::ifstream& In, FAnimationClipBinaryHeader& OutHeader) const
{
    return ReadUInt32LE(In, OutHeader.MagicNumber)
        && ReadUInt32LE(In, OutHeader.Version)
        && ReadUInt32LE(In, OutHeader.BoneTrackCount)
        && ReadUInt32LE(In, OutHeader.ShapeKeyTrackCount)
        && ReadUInt32LE(In, OutHeader.CurveCount)
        && ReadUInt32LE(In, OutHeader.KeyCount)
        && ReadUInt64LE(In, OutHeader.SourceFileWriteTime);
}

bool FAnimationClipSerializer::ReadCurveKey(std::ifstream& In, FAnimationCurveKey& OutKey) const
{
    return ReadFloatLE(In, OutKey.TimeSeconds)
        && ReadFloatLE(In, OutKey.Value);
}

bool FAnimationClipSerializer::ReadFloatCurve(std::ifstream& In, FAnimationFloatCurve& OutCurve) const
{
    int32 Channel = 0;
    uint32 KeyCount = 0;
    if (!ReadInt32LE(In, Channel) ||
        !ReadFloatLE(In, OutCurve.DefaultValue) ||
        !ReadBool(In, OutCurve.bHasCurve) ||
        !ReadUInt32LE(In, KeyCount) ||
        KeyCount > MAX_ANIMATION_KEY_COUNT)
    {
        return false;
    }

    OutCurve.Channel = static_cast<EAnimationCurveChannel>(Channel);
    OutCurve.Keys.resize(KeyCount);
    for (FAnimationCurveKey& Key : OutCurve.Keys)
    {
        if (!ReadCurveKey(In, Key))
        {
            return false;
        }
    }
    return true;
}

bool FAnimationClipSerializer::ReadBoneTrack(std::ifstream& In, FBoneAnimationTrack& OutTrack) const
{
    if (!ReadString(In, OutTrack.BoneName) ||
        !ReadVector(In, OutTrack.DefaultTranslation) ||
        !ReadVector(In, OutTrack.DefaultRotationEuler) ||
        !ReadVector(In, OutTrack.DefaultScale))
    {
        return false;
    }

    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        if (!ReadFloatCurve(In, OutTrack.Translation[Axis]) ||
            !ReadFloatCurve(In, OutTrack.Rotation[Axis]) ||
            !ReadFloatCurve(In, OutTrack.Scale[Axis]))
        {
            return false;
        }
    }
    return true;
}

bool FAnimationClipSerializer::ReadShapeKeyTrack(std::ifstream& In, FShapeKeyAnimationTrack& OutTrack) const
{
    return ReadString(In, OutTrack.MeshNodeName)
        && ReadString(In, OutTrack.ShapeKeyName)
        && ReadInt32LE(In, OutTrack.ShapeKeyIndex)
        && ReadFloatCurve(In, OutTrack.WeightCurve);
}

bool FAnimationClipSerializer::SaveAnimationClip(const FString& BinaryPath, const FString& SourcePath, const FAnimationClip& Data)
{
    std::ofstream Out(std::filesystem::path(FPaths::ToAbsolute(FPaths::ToWide(BinaryPath))), std::ios::binary);
    if (!Out.is_open())
    {
        return false;
    }

    uint32 CurveCount = 0;
    uint32 KeyCount = 0;
    CountClipData(Data, CurveCount, KeyCount);

    FAnimationClipBinaryHeader Header;
    Header.MagicNumber = ANIMATION_CLIP_BINARY_MAGIC;
    Header.Version = ANIMATION_CLIP_BINARY_VERSION;
    Header.BoneTrackCount = static_cast<uint32>(Data.BoneTracks.size());
    Header.ShapeKeyTrackCount = static_cast<uint32>(Data.ShapeKeyTracks.size());
    Header.CurveCount = CurveCount;
    Header.KeyCount = KeyCount;
    Header.SourceFileWriteTime = GetFileWriteTimeTicks(SourcePath);

    if (!IsValidHeader(Header))
    {
        return false;
    }

    WriteHeader(Out, Header);
    WriteString(Out, Data.Name);
    WriteString(Out, Data.SourcePath);
    WriteString(Out, Data.SkeletonSourcePath);
    WriteFloatLE(Out, Data.StartTimeSeconds);
    WriteFloatLE(Out, Data.EndTimeSeconds);
    WriteFloatLE(Out, Data.DurationSeconds);
    WriteFloatLE(Out, Data.SourceFrameRate);

    WriteUInt32LE(Out, Header.BoneTrackCount);
    for (const FBoneAnimationTrack& Track : Data.BoneTracks)
    {
        WriteBoneTrack(Out, Track);
    }

    WriteUInt32LE(Out, Header.ShapeKeyTrackCount);
    for (const FShapeKeyAnimationTrack& Track : Data.ShapeKeyTracks)
    {
        WriteShapeKeyTrack(Out, Track);
    }

    return Out.good();
}

bool FAnimationClipSerializer::LoadAnimationClip(const FString& BinaryPath, FAnimationClip& OutData)
{
    std::ifstream In(std::filesystem::path(FPaths::ToAbsolute(FPaths::ToWide(BinaryPath))), std::ios::binary);
    if (!In.is_open())
    {
        return false;
    }

    FAnimationClipBinaryHeader Header;
    if (!ReadHeader(In, Header) || !IsValidHeader(Header))
    {
        return false;
    }

    if (!ReadString(In, OutData.Name) ||
        !ReadString(In, OutData.SourcePath) ||
        !ReadString(In, OutData.SkeletonSourcePath) ||
        !ReadFloatLE(In, OutData.StartTimeSeconds) ||
        !ReadFloatLE(In, OutData.EndTimeSeconds) ||
        !ReadFloatLE(In, OutData.DurationSeconds) ||
        !ReadFloatLE(In, OutData.SourceFrameRate))
    {
        return false;
    }

    uint32 BoneTrackCount = 0;
    if (!ReadUInt32LE(In, BoneTrackCount) || BoneTrackCount != Header.BoneTrackCount)
    {
        return false;
    }

    OutData.BoneTracks.resize(BoneTrackCount);
    for (FBoneAnimationTrack& Track : OutData.BoneTracks)
    {
        if (!ReadBoneTrack(In, Track))
        {
            return false;
        }
    }

    uint32 ShapeKeyTrackCount = 0;
    if (!ReadUInt32LE(In, ShapeKeyTrackCount) || ShapeKeyTrackCount != Header.ShapeKeyTrackCount)
    {
        return false;
    }

    OutData.ShapeKeyTracks.resize(ShapeKeyTrackCount);
    for (FShapeKeyAnimationTrack& Track : OutData.ShapeKeyTracks)
    {
        if (!ReadShapeKeyTrack(In, Track))
        {
            return false;
        }
    }

    return In.good();
}

bool FAnimationClipSerializer::ReadAnimationClipHeader(const FString& BinaryPath, FAnimationClipBinaryHeader& OutHeader) const
{
    std::ifstream In(std::filesystem::path(FPaths::ToAbsolute(FPaths::ToWide(BinaryPath))), std::ios::binary);
    if (!In.is_open())
    {
        return false;
    }

    return ReadHeader(In, OutHeader) && IsValidHeader(OutHeader);
}
