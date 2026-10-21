# KRAFTON JUNGLE GameTechLab 3기

## C++ / DirectX 11 자체 엔진 및 게임 클라이언트 포트폴리오

본 저장소는 **KRAFTON JUNGLE GameTechLab 3기** 과정에서 14주간 주차별 팀 프로젝트로 개발한 C++/DirectX 기반 자체 엔진 및 게임 클라이언트 작업을 한곳에 모은 포트폴리오입니다.

> 이 저장소는 한 팀이 하나의 엔진을 14주 동안 연속 개발한 단일 프로젝트가 아닙니다. 주차마다 팀 구성, 공통 기반 코드, 과제와 담당 범위가 달랐으며, 아래에는 제가 직접 구현하거나 수정한 범위만 구분해 작성했습니다.

- 개발 기록 기간: 2026.03.09 ~ 2026.06.09
- GitHub: [esc10946](https://github.com/esc10946)
- 관심 분야: 게임 클라이언트, 자체 엔진, 렌더링, 개발 도구
- 주요 언어: C++, HLSL, Lua, Python
- 주요 기술: DirectX 11, Win32, ImGui, RmlUi, PhysX, Git

## 핵심 역량 요약

| 영역 | 대표 경험 | 관련 주차 |
|---|---|---:|
| 자료구조·엔진 최적화 | Loose Octree, dirty 기반 동적 갱신, Tick Function | Week05 |
| DX11 렌더링·자원 관리 | Shadow Atlas, Point Light Cube Array, DOF 후처리 | Week08, Week13 |
| 엔진 구조·리팩토링 | Reflection Registry, Property System, 코드 생성 | Week11 |
| 개발 도구 | Skeletal Mesh Viewer, Animation Viewer, RmlUi Preview | Week10, Week11, Week14 |
| 콘텐츠 런타임 | Particle Emitter Instance, Mesh·Beam 확장 | Week12 |
| 캐릭터·게임 클라이언트 | Animation Montage Blend, HUD 데이터 연결, 게임패드 | Week14 |
| 자산·에디터 파이프라인 | Static Mesh Section Material, MTL 연동, 장면 복원 | Week04 |

## 14주 기여 요약

아래 표는 각 팀 프로젝트 전체 기능이 아니라, 개인 변경 이력에서 확인한 핵심 기여만 요약한 것입니다. 파일 링크는 주차별 최종 코드입니다

| 주차 | 확인된 개인 기여 | 대표 코드·변경 |
|---:|---|---|
| [Week01](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week01) | 게임 장면 구조, 점수·상태 관리, FMOD Sound Manager, Trail Particle | [`UGameScene.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week01/JungleEntranceExam/UGameScene.cpp) · [`USoundManager.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week01/JungleEntranceExam/USoundManager.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/1e9a114dacf1c9a64476f8a6d2253bbf7e95e9b3) |
| [Week02](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week02) | App Context 구조, Picking Service, 객체 선택·기즈모, 장면·카메라 직렬화 | [`AppContext.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week02/KcraftonJungle_WEEK02/AppContext.cpp) · [`PickingService.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week02/KcraftonJungle_WEEK02/Engine/Services/PickingService.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/13359749d964dcb777f297b9e2e4484fede65030) |
| [Week03](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week03) | Texture Manager, Text Component·Mesh Builder, 한글 텍스트, Text Batching | [`TextBatcherComponent.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week03/Source/Engine/Private/Classes/Components/TextBatcherComponent.cpp) · [`TextureManager.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week03/Source/Engine/Private/Classes/TextureManager.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/fad3a9b4543549360d0e5276b66b1b6179450507) |
| [Week04](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week04) | Static Mesh Component·Actor 연동, Section별 Material override, MTL·장면 복원 보완 | [`StaticMeshComponent.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week04/W4_Jungle_Team4/Source/Engine/Component/StaticMeshComponent.cpp) · [`MtlParser.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week04/W4_Jungle_Team4/Source/Engine/AssetPipeline/Obj/MtlParser.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/d33190b30de3f265475909bd80d2cde8c267bbe3) |
| [Week05](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week05) | Loose Octree, Spatial Partition, dirty 갱신, Tick Function | [`Octree.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week05/KraftonEngine/Source/Engine/Collision/Octree.cpp) · [`TickFunction.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week05/KraftonEngine/Source/Engine/Core/TickFunction.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/fcb73bbc8b31aa2f9dcfe6cb2884ab606da1d6ab) |
| [Week06](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week06) | Swept Projectile 충돌, 충돌 지점 Decal, Spotlight와 에디터 연동 | [`ProjectileMovementComponent.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week06/KraftonEngine/Source/Engine/Component/ProjectileMovementComponent.cpp) · [`EggActor.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week06/KraftonEngine/Source/Engine/GameFramework/EggActor.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/f2be1cae2fa2595f4c3d04e8d340d2ff7a76286b) |
| [Week07](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week07) | UberLit 셰이딩 모델, World Normal View Mode, MTL Specular·Spot Falloff | [`UberLit.hlsl`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week07/KraftonEngine/Shaders/UberLit.hlsl) · [`RenderConstants.h`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week07/KraftonEngine/Source/Engine/Render/Pipeline/RenderConstants.h) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/461c64955e755d85a803da1cd29eea4128ea6a4b) |
| [Week08](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week08) | Shadow Atlas, Point Light Cube Array, 동적 tile 할당과 자원 수명 관리 | [`ShadowAtlasManager.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week08/NipsEngine/Source/Engine/Render/Resource/ShadowAtlasManager.cpp) · [`ShadowPass.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week08/NipsEngine/Source/Engine/Render/Renderer/RenderFlow/ShadowPass.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/3ad5eb43ab56d2ebe561073f6b1232f46bac76ac) |
| [Week09](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week09) | Lua Script Component·Manager 연동, Hot Reload, 게임플레이와 Camera Shake | [`ScriptManager.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week09/NipsEngine/Source/Engine/Runtime/Script/ScriptManager.cpp) · [`CameraShakeBase.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week09/NipsEngine/Source/Engine/Camera/CameraShakeBase.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/fbc11de996c14636f38739b24c4c67eff779a451) |
| [Week10](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week10) | Asset Viewer, Skeletal Mesh Viewer, Preview Viewport와 Bone 시각화 | [`SkeletalMeshViewer.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week10/PacificEngine/Source/Editor/Viewport/SkeletalMeshViewer.cpp) · [`AssetViewerManager.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week10/PacificEngine/Source/Editor/Viewport/AssetViewerManager.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/2451184fef3a2ef0f9bc5b9c991594c102a8717d) |
| [Week11](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week11) | Reflection Property System, 코드 생성, Asset·Array·Function, Animation Viewer | [`ReflectionRegistry.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week11/JSEngine/Source/Engine/Object/ReflectionRegistry.cpp) · [`GenerateReflection.py`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week11/JSEngine/BuildTools/Scripts/GenerateReflection.py) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/0212cdfd63c965025482ebe76eec36063b8e52ad) |
| [Week12](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week12) | Particle Emitter Instance, payload, Mesh·Beam·Noise Runtime 확장 | [`ParticleEmitterInstance.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week12/KraftonEngine/Source/Engine/Particles/Runtime/ParticleEmitterInstance.cpp) · [`ParticleSceneProxy.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week12/KraftonEngine/Source/Engine/Render/Proxy/ParticleSceneProxy.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/666c6452aac5c5e17fdc3171f7acbe93db397ca0) |
| [Week13](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week13) | Depth of Field Pass, PhysX Vehicle Runtime 연동과 디버그 UI | [`DepthOfFieldPass.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week13/KraftonEngine/Source/Engine/Render/RenderPass/DepthOfFieldPass.cpp) · [`VehicleMovementComponent.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week13/KraftonEngine/Source/Engine/Component/VehicleMovementComponent.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/be806a3e15f36534710f58a894e43ac3ba023f1c) |
| [Week14](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/tree/main/Week14) | RmlUi Hot Reload·Preview, 이벤트 기반 HUD, Montage Blend, 전투·게임패드 연동 | [`UIManager.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week14/KraftonEngine/Source/Engine/UI/UIManager.cpp) · [`AnimMontageInstance.cpp`](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/blob/main/Week14/KraftonEngine/Source/Engine/Animation/Montage/AnimMontageInstance.cpp) · [변경](https://github.com/esc10946/KraftonJungle-Portfolio-Engine/commit/5bf22edd7bca550819f71ecfaf5d1f8ba6bccaca) |

## 팀 코드와 외부 코드의 범위

### 팀 공동 개발

- 각 주차의 Renderer, Physics, Editor, Gameplay와 Asset Pipeline에는 팀원 구현이 함께 포함되어 있습니다.
- 제가 구현한 기능도 팀원이 만든 공통 엔진 기반과 인터페이스 위에서 동작하며, 이후 다른 팀원의 안정화와 리팩토링이 포함된 파일이 있습니다.
- 브랜치 통합과 충돌 해결은 협업 기여로 기록하되, 병합된 기능의 최초 구현자로 표기하지 않습니다.

### 외부 라이브러리 및 자산

- `ThirdParty` 아래의 ImGui, Lua/LuaJIT, PhysX, NvCloth, FBX SDK, RmlUi 등은 외부 라이브러리입니다.
- 외부 라이브러리 자체가 아니라 엔진 수명 주기, 렌더링, Reflection, Script와 Editor에 연결한 코드만 개인 기여로 설명합니다.
- FBX, Texture, Sound, Model과 기타 Binary Asset은 기능 검증용으로 포함된 항목이 있으며, 원본 제작을 개인 기여로 주장하지 않습니다.

## 저장소 구조

```text
KraftonJungle-Portfolio-Engine/
├─ Week01/  # 게임 루프와 장면·사운드
├─ Week02/  # 에디터와 피킹·직렬화
├─ Week03/  # 텍스트와 텍스처·배칭
├─ Week04/  # Static Mesh와 Material 편집
├─ Week05/  # 공간 분할과 Tick
├─ Week06/  # Projectile 충돌과 Decal
├─ Week07/  # 조명·셰이딩
├─ Week08/  # Shadow Atlas
├─ Week09/  # Lua와 게임플레이
├─ Week10/  # Skeletal Mesh Viewer
├─ Week11/  # Reflection과 Animation Editor
├─ Week12/  # Particle Runtime
├─ Week13/  # DOF와 PhysX Vehicle
└─ Week14/  # Runtime UI와 캐릭터·전투 통합
```

