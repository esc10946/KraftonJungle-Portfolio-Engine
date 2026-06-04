#include "Game/Lua/GameLuaBindings.h"

#include "sol/sol.hpp"

#include "Engine/Runtime/Engine.h"
#include "Engine/Runtime/EngineInitHooks.h"
#include "Lua/LuaScriptManager.h"

// ============================================================
// 게임-특화 Lua 바인딩 등록 위치 — 현재는 비어 있음.
//
// Engine 의 FLuaScriptManager 가 등록하는 일반 binding (AActor / APawn / FVector /
// UWorld / Anim 등) 만으로 동작하지 않는 game-specific usertype (ACarPawn /
// AGameStateXxx / 전용 enum 등) 이 도입되면 여기에 new_usertype 으로 추가한다.
//
// 호출 시점: UEngine::Init() 이 FLuaScriptManager::Initialize() 를 끝낸 직후.
// 등록은 EngineInitHooks 에 자동으로 걸려 GameEngine / EditorEngine 두 엔트리 모두
// 같은 바인딩이 적용된다 (PIE 호환).
// ============================================================
void RegisterGameLuaBindings(sol::state& Lua)
{
	// "Engine" 테이블은 엔진 측 바인딩(FLuaScriptManager)이 이미 만들어 두지만,
	// 등록 순서에 의존하지 않도록 없으면 생성한다.
	sol::table Engine = Lua["Engine"].valid() ? Lua["Engine"] : Lua.create_named_table("Engine");

	// Engine.OpenScene("MapName" 또는 "Scene/Foo.Scene") — 다음 frame Tick 끝에서
	// active world 를 destroy 하고 해당 scene 으로 교체한다. World->Tick 바깥에서
	// 일어나므로 버튼 클릭(Lua) 콜백에서 호출해도 use-after-free 가 없다.
	Engine.set_function(
		"OpenScene",
		[](const FString& SceneNameOrPath)
		{
			if (GEngine)
			{
				GEngine->RequestTransitionToScene(SceneNameOrPath);
			}
		}
	);
}

// 자기-등록 — Editor / Game 측이 RegisterGameLuaBindings 함수명을 모르고도
// FEngineInitHooks::RunAll() 한 번이면 호출되도록 static initializer 로 등록.
namespace
{
	void RunRegisterGameLuaBindings()
	{
		RegisterGameLuaBindings(FLuaScriptManager::GetState());
	}

	struct GameLuaBindingsAutoReg
	{
		GameLuaBindingsAutoReg() { FEngineInitHooks::Register(&RunRegisterGameLuaBindings); }
	};

	static GameLuaBindingsAutoReg gAutoReg;
}
