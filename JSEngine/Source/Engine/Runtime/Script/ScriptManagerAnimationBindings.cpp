#include "Runtime/Script/ScriptManager.h"

#include "Animation/ActorSequence.h"
#include "Animation/AnimationStateMachine.h"
#include "Asset/CurveFloatAsset.h"
#include "Object/Object.h"
#include "Runtime/Script/ScriptComponent.h"
#include "Runtime/Script/ScriptUtils.h"

void FScriptManager::BindAnimationTypes()
{
    LUA_BEGIN_TYPE_NO_CTOR_BASE(GLuaState, UCurveFloatAsset, "CurveFloatAsset", UObject)
    LUA_METHOD(Evaluate, Evaluate);
    LUA_METHOD(GetAssetPath, GetAssetPath);
    LUA_END_TYPE();

    LUA_BEGIN_TYPE_NO_CTOR_BASE(GLuaState, UActorSequence, "ActorSequence", UObject)
    LUA_FIELD(StartTime, StartTime);
    LUA_FIELD(Duration, Duration);
    LUA_FIELD(Loop, bLoop);
    LUA_END_TYPE();

    LUA_BEGIN_TYPE_NO_CTOR_BASE(GLuaState, UActorSequencePlayer, "ActorSequencePlayer", UObject)
    LUA_METHOD(Play, Play);
    LUA_METHOD(Pause, Pause);
    LUA_METHOD(Stop, Stop);
    LUA_METHOD(SetCurrentTime, SetCurrentTime);
    LUA_METHOD(GetCurrentTime, GetCurrentTime);
    LUA_METHOD(IsPlaying, IsPlaying);
    LUA_END_TYPE();

    LUA_BEGIN_TYPE_NO_CTOR(GLuaState, FLuaTimeline, "LuaTimeline")
    LUA_METHOD(Play, Play);
    LUA_METHOD(Pause, Pause);
    LUA_METHOD(Stop, Stop);
    LUA_METHOD(Tick, Tick);
    LUA_METHOD(SetPlayRate, SetPlayRate);
    LUA_METHOD(SetLoop, SetLoop);
    LUA_METHOD(SetCurrentTime, SetCurrentTime);
    LUA_METHOD(GetCurrentTime, GetCurrentTime);
    LUA_METHOD(AddFloatTrack, AddFloatTrack);
    LUA_METHOD(ClearTracks, ClearTracks);
    LUA_END_TYPE();

    LUA_BEGIN_TYPE_NO_CTOR_BASE(GLuaState, UAnimStateMachineAsset, "AnimStateMachineAsset", UObject)
    LUA_SET(SetEntryState, [](UAnimStateMachineAsset& Self, const FString& StateName)
    {
        Self.SetEntryState(FName(StateName));
    });
    LUA_SET(AddState, [](UAnimStateMachineAsset& Self, const FString& StateName, const FString& AnimationName, bool bLoop)
    {
        return Self.AddState(FName(StateName), FName(AnimationName), bLoop);
    });
    LUA_SET(AddStateWithPath, [](
        UAnimStateMachineAsset& Self,
        const FString& StateName,
        const FString& AnimationName,
        const FString& AnimationPath,
        bool bLoop)
    {
        return Self.AddState(FName(StateName), FName(AnimationName), bLoop, AnimationPath);
    });
    LUA_SET(SetStateAnimationPath, [](
        UAnimStateMachineAsset& Self,
        const FString& StateName,
        const FString& AnimationPath)
    {
        return Self.SetStateAnimationPath(FName(StateName), AnimationPath);
    });
    LUA_SET(AddTransition, [](
        UAnimStateMachineAsset& Self,
        const FString& FromState,
        const FString& ToState,
        const FString& ConditionName,
        float BlendTime,
        int32 Priority)
    {
        return Self.AddTransition(
            FName(FromState),
            FName(ToState),
            FName(ConditionName),
            BlendTime,
            Priority);
    });
    LUA_SET(Validate, [](const UAnimStateMachineAsset& Self)
    {
        return Self.Validate();
    });
    LUA_END_TYPE();

    GLuaState->set_function("CreateAnimStateMachineAsset", []() -> UAnimStateMachineAsset*
    {
        return UObjectManager::Get().CreateObject<UAnimStateMachineAsset>();
    });
}
