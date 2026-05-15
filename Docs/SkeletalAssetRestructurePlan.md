# Skeletal Asset Restructure Plan

작성일: 2026-05-16

## 목적

FBX 하나에서 여러 skeletal mesh, skeleton, animation clip을 모두 추출하고, skeleton을 독립 에셋으로 분리한다. 임포트된 바이너리 에셋만 에디터/컴포넌트에서 선택 가능하게 하며, skeletal mesh와 animation은 대상 skeleton을 상대경로로 참조한다.

기존 캐시와의 호환성은 고려하지 않는다. 대신 각 단계가 빌드 가능한 상태로 끝나도록 작게 나누고, 캐시 일괄 삭제 후 재임포트하는 전제를 둔다.

## 현재 구조 요약

- FBX 로더: `JSEngine/Source/Engine/Asset/FbxImporter.*`
- skeletal mesh 데이터: `FSkeletalMesh`가 `Bones`, `Sockets`, `Sections`, `MaterialSlots`를 직접 소유
- animation 데이터: `FAnimationClip`에 `SkeletonSourcePath` 필드는 이미 있으나 현재 로드 흐름은 FBX 첫 clip 중심
- skeletal mesh 로드: `FResourceManager::LoadSkeletalMesh(Path)` -> `FSkeletalMeshLoadService`
- animation 로드: `FResourceManager::LoadAnimationClip(Path)` -> `FAnimationClipLoadService`
- 캐시 경로: 현재 skeletal mesh는 `Asset/SkeletalMesh/Bin/{FBX stem}.bin`, animation은 `Asset/Animation/Bin/{FBX stem}.bin`
- 에디터 선택 목록: `FEditorAssetService`가 `.fbx`와 리소스 매니저 목록을 섞어서 skeletal mesh 후보로 노출
- 주요 런타임 의존: `USkeletalMesh::GetBones()`, `USkinnedMeshComponent`, skeleton/bone viewer, socket editor, overlay collector

## 목표 정책

- Skeleton 바이너리 이름: `{FBX파일명}_skeleton_{스켈레톤루트FbxNode이름}.bin`
- Animation 바이너리 이름: `{FBX파일명}_anim_{애니메이션클립명}.bin`
- 기본 캐시 위치: FBX 파일과 같은 폴더
- FBX 임포트는 명시적 명령에서만 수행한다. Contents drawer에서 FBX를 더블클릭했을 때 명시적 import를 실행한 뒤, import 결과 skeletal mesh를 viewer로 연다.
- 이미 import된 FBX를 더블클릭한 경우에는 재임포트하지 않고, 이미 생성된 skeletal mesh asset 중 deterministic 기준의 임의 1개를 viewer로 연다.
- 기존 Contents Browser는 `.fbx` viewport drag/drop 배치를 지원하므로, FBX drag/drop도 명시적 import 동작으로 취급한다.
- FBX 임포트 시 skeleton, skeletal mesh, animation clip 조합을 모두 추출한다.
- 임포트되지 않은 FBX 내부 항목은 `SkeletalMeshComponent` 등 선택 UI에 노출하지 않음
- `FSkeletalMesh`와 `FAnimationClip`은 target skeleton 상대경로를 저장
- `LoadSkeletalMesh("file.fbx")`: 첫 번째 skeletal mesh 반환
- `LoadSkeletalMesh("file.fbx", "skeletonName")`: 지정 skeleton root FbxNode 이름에 해당하는 mesh 반환
- 단일 FBX 파일 내 같은 skeleton을 쓰는 여러 skeletal mesh는 하나의 캐릭터 파츠로 보고 material slot 기준으로 병합
- socket은 skeletal mesh별 데이터가 아니라 skeleton 공용 에셋에 저장
- animation stack 하나가 여러 skeleton에 적용 가능한 경우는 고려하지 않는다. 구현이 단순한 단일 target skeleton 정책을 사용한다.
- bone index는 현재 `uint8` 기반 제한을 유지한다.

## 안전 원칙

1. 새 타입과 직렬화를 먼저 추가하고 기존 런타임 API를 얇은 호환 레이어로 유지한다.
2. 에디터 선택 정책 변경은 import/cache 생성 로직이 안정화된 뒤 적용한다.
3. 기존 `.bin` 호환은 버리되, 파일명 충돌과 상대경로 계산은 별도 유틸로 격리한다.
4. 각 단계마다 `GenerateProjectFiles.bat` 또는 `ReleaseBuild.bat` 수행 지점을 둔다.
5. 단계 중 발견되는 구조 위험은 즉시 보고하고 계획을 갱신한다.
6. lazy import는 금지한다. 로드 API는 이미 import된 결과만 읽거나, 명시 import 결과가 없으면 실패/안내하도록 만든다.

## 단계별 작업 계획

### 0. 기준선 확인

작업 전 현재 브랜치와 빌드 상태를 확인한다.

- `git status --short`
- `GenerateProjectFiles.bat`
- `ReleaseBuild.bat`

검증 지점:

- 프로젝트 파일 재생성 성공
- 에디터 Release 빌드 성공
- 현재 실패가 있다면 asset 구조 변경 전 별도 기준선 이슈로 기록

### 1. Skeleton 에셋 타입 도입

추가/변경 후보:

- `JSEngine/Source/Engine/Asset/SkeletonTypes.h`
- `JSEngine/Source/Engine/Asset/SkeletonAsset.h/.cpp`
- `JSEngine/Source/Engine/Asset/SkeletonSerializer.h/.cpp`
- `JSEngine/Source/Engine/Core/SkeletonLoadService.h/.cpp`
- `FResourceManager`에 skeleton map/list/load API 추가

설계:

- `FSkeleton`은 `PathFileName`, `SourcePath`, `RootNodeName`, `Bones`를 소유
- `FBoneInfo`는 우선 현재 `SkeletalMeshTypes.h`에서 공용 헤더로 이동하거나 skeleton 헤더로 이전
- `USkeletonAsset` 또는 `USkeleton` 명칭 중 기존 `UObject` 네이밍과 충돌이 적은 이름 선택
- skeleton 바이너리 헤더에는 version, source write time, bone count, root node name 저장
- `FSkeletalMesh`에는 `SkeletonSourcePath` 문자열 추가
- `FSkeletalMeshSocket` 또는 equivalent socket 타입은 skeleton 공용 데이터로 이전
- 전환 기간에는 기존 `USkeletalMesh::FindSocket/HasSocket` API가 target skeleton socket을 조회하도록 bridge 유지

검증 지점:

- `GenerateProjectFiles.bat`
- `ReleaseBuild.bat`
- 아직 로더 동작 변경 전이므로 기존 skeletal mesh 표시가 깨지지 않는지 간단 실행 확인

### 2. 캐시 경로 정책 분리

변경 후보:

- `JSEngine/Source/Engine/Core/AssetPathPolicy.*`

추가 API 후보:

- `MakeSiblingSkeletonBinaryPath(SourceFbxPath, SkeletonRootName)`
- `MakeSiblingSkeletalMeshBinaryPath(SourceFbxPath, SkeletonRootName 또는 MeshName)`
- `MakeSiblingAnimationClipBinaryPath(SourceFbxPath, ClipName)`
- `SanitizeImportedAssetToken(Token)`
- `MakeRelativeAssetPath(FromAssetPath, ToAssetPath)`

정책:

- 기본 캐시는 FBX sibling `.bin`으로 생성
- 이름 토큰은 파일 시스템 안전 문자로 sanitize
- 같은 FBX 안에 동일 root/clip 이름이 중복되면 `_1`, `_2` suffix로 충돌 회피
- 기존 `Asset/SkeletalMesh/Bin`와 `Asset/Animation/Bin` 경로는 신규 임포트에서 사용하지 않음

검증 지점:

- `ReleaseBuild.bat`
- 경로 유틸 단위 호출이 가능한 테스트/임시 로그 지점이 있다면 샘플 문자열로 경로 확인

### 3. FBX scene 분석 결과 모델 추가

변경 후보:

- `FbxImporter.h/.cpp`

추가 구조 후보:

- `FFbxSkeletonImportDesc`
- `FFbxSkeletalMeshImportDesc`
- `FFbxAnimationImportDesc`
- `FFbxImportedAssetSet`

목표:

- FBX를 한 번 열어 skeleton root 후보, skinned mesh 노드, rigid attached mesh 노드, animation stack을 수집
- mesh가 어떤 skeleton root에 귀속되는지 cluster link의 최상위 skeleton/root node 기준으로 결정
- animation은 적용 가능한 skeleton별로 clip asset을 생성할 수 있게 매핑
- 기존 `InspectMeshContent`는 유지하되 내부 구현은 새 분석 결과를 활용 가능

검증 지점:

- `ReleaseBuild.bat`
- 기존 `LoadSkeletalMesh("Asset/...fbx")`가 여전히 첫 skeletal mesh를 반환하도록 임시 bridge 유지

### 4. Skeleton 추출과 저장

변경 후보:

- `FbxImporter.*`
- `SkeletonSerializer.*`
- `SkeletonLoadService.*`
- `ResourceManager.*`

작업:

- FBX skeleton root별 `FSkeleton` 생성
- bind pose, local/global bind transform, inverse bind pose 계산을 skeleton 쪽으로 이동
- socket 데이터 저장 위치를 skeleton binary로 이동
- skeleton binary 저장
- skeleton load/find/register API 추가
- skeleton cache freshness는 FBX source write time으로 검사

검증 지점:

- `ReleaseBuild.bat`
- 캐시 삭제 후 샘플 FBX 임포트
- 생성 파일 확인: `{fbxStem}_skeleton_{root}.bin`
- skeleton bone count/root name 로그 확인

### 5. Skeletal mesh를 skeleton 종속 에셋으로 변경

변경 후보:

- `FSkeletalMesh`, `USkeletalMesh`
- `BinarySerializer`
- `FSkeletalMeshLoadService`
- `FResourceManager::LoadSkeletalMesh` overload
- `USkinnedMeshComponent`
- viewer/socket editor/overlay collector

작업:

- `FSkeletalMesh::Bones`는 최종적으로 제거하거나, 전환 기간 동안 skeleton에서 읽는 deprecated cache로 둠
- `FSkeletalMesh::SkeletonSourcePath` 저장/로드
- `USkeletalMesh`가 target skeleton 포인터 또는 경로를 통해 bone 정보 접근
- `USkeletalMesh::GetBones()` 등 기존 호출부는 가능하면 유지해 내부에서 skeleton을 반환
- `LoadSkeletalMesh(Path)`는 이미 import된 FBX manifest/asset set의 첫 merged mesh 반환
- `LoadSkeletalMesh(Path, SkeletonName)` overload 추가. `SkeletonName`은 skeleton root FbxNode 이름으로 해석
- manifest/cache가 없으면 FBX를 자동 import하지 않고 실패 또는 import 필요 로그를 남김
- mesh binary 이름은 skeleton root 또는 mesh name을 포함해 충돌 방지

검증 지점:

- `ReleaseBuild.bat`
- 기본 캐시 삭제 후 단일 skeletal mesh FBX 로드
- 기존 viewer에서 bone tree, socket editor, skinning, outline overlay 확인
- 다중 skeletal mesh FBX가 있다면 첫 mesh 반환 확인

### 6. Animation clip을 skeleton 종속 에셋으로 변경

변경 후보:

- `FAnimationClip`
- `AnimationClipSerializer`
- `FAnimationClipLoadService`
- `FbxImporter::LoadAnimations`
- 필요 시 에디터 animation asset 목록

작업:

- `FAnimationClip::SkeletonSourcePath`를 필수 데이터로 취급
- clip binary 이름을 `{fbxStem}_anim_{clipName}.bin`으로 변경
- animation stack이 여러 개인 경우 전부 저장
- animation stack 하나가 여러 skeleton에 적용 가능한 경우는 고려하지 않고, 구현이 단순한 target skeleton 하나에 연결
- load 시 target skeleton과 bone track 이름 매칭 검증 로그 추가

검증 지점:

- `ReleaseBuild.bat`
- multi-stack FBX 임포트 후 clip별 `.bin` 생성 확인
- `LoadAnimationClip`로 특정 clip binary 로드 확인
- skeleton path가 상대경로로 직렬화/역직렬화되는지 확인

### 7. 통합 임포트 API와 import manifest 도입

추가 후보:

- `FImportedFbxManifest` 또는 `FImportedFbxAssetIndex`
- manifest 파일은 같은 폴더에 `{fbxStem}_import.json` 또는 별도 `.import` 파일

목표:

- 어떤 FBX 내부 항목을 실제로 import했는지 기록
- 에디터와 컴포넌트 선택 목록은 manifest 또는 생성된 `.bin` 기준으로만 구성
- FBX 원본만 있고 import되지 않은 내부 mesh/animation은 선택 불가
- Contents drawer에서 FBX 더블클릭 시 명시적 import 명령 실행
- Contents drawer에서 FBX를 viewport로 drag/drop하면 명시적 import 후 import된 skeletal mesh를 일괄 배치
- import 성공 후 skeleton, merged skeletal mesh, animation clip manifest entry를 생성
- load API는 manifest/cache를 소비만 하고 FBX 원본을 자동 import하지 않음
- FBX 더블클릭은 편의 동작으로 취급한다. 미임포트 FBX는 import 후 viewer open, 이미 import된 FBX는 재임포트 없이 기존 imported skeletal mesh viewer open.

검토 필요:

- manifest 파일을 꼭 둘지, `.bin` 스캔만으로 충분한지 결정 필요
- 병합 결과만 보관한다. 개별 파츠 mesh asset은 별도 요구가 생기기 전까지 만들지 않는다.

검증 지점:

- `ReleaseBuild.bat`
- 캐시 삭제 후 import 실행 전에는 skeletal mesh 선택 목록에 FBX 내부 항목이 나오지 않는지 확인
- import 후 생성된 skeletal mesh/animation만 목록에 노출되는지 확인
- FBX 더블클릭 외 단순 component load 경로에서는 import가 발생하지 않는지 로그로 확인
- FBX viewport drag/drop은 기존 지원 동작을 유지하되, import 후 imported skeletal mesh 배치로 전환되는지 확인

### 8. Material slot 기준 mesh 병합

참고 코드:

- `C:\Projects\Jungle_Week10_Team6\PacificEngine\Source\Engine\Mesh\FBXImporter.*`

작업:

- 같은 skeleton root에 속한 skeletal mesh 후보를 그룹화
- 단일 FBX의 여러 skeletal mesh는 하나의 캐릭터 파츠로 보고 material slot 이름/순서를 기준으로 section과 slot 병합
- vertex/index offset 보정
- bone index는 같은 skeleton 기준이므로 유지
- rigid attached mesh 처리는 현재 레포의 2-pass 정책과 참고 레포의 추출 정책을 비교해 보수적으로 병합
- bone index 타입은 현행 `uint8` 제한을 유지하며, 256개 초과 bone은 기존 정책대로 경고/스킵 처리

검증 지점:

- `ReleaseBuild.bat`
- 같은 skeleton 다중 mesh FBX에서 section/material slot 수 확인
- 병합 전후 vertex/index count 로그 확인
- viewer에서 material slot별 rendering 확인

### 9. 에디터 선택 정책 변경

변경 후보:

- `FEditorAssetService`
- `EditorPropertyWidget`
- `EditorMainPanelPlacement`
- `EditorViewer`
- `EditorContentBrowserWidget` import 액션이 있다면 해당 부분

작업:

- skeletal mesh 선택 목록은 `.fbx` 원본 대신 imported skeletal mesh `.bin` 또는 manifest entry 기준
- animation 선택 목록도 imported animation `.bin` 또는 manifest entry 기준
- `SkeletalMeshComponent` 문자열 프로퍼티에는 import된 mesh asset path 저장
- Contents drawer에서 FBX 더블클릭 시 미임포트 상태면 import 후 viewer open
- Contents drawer에서 FBX 더블클릭 시 이미 import된 상태면 재임포트 없이 기존 imported skeletal mesh viewer open
- Contents drawer에서 FBX 원본을 viewport로 drag/drop하면 import 후 imported skeletal mesh를 일괄 배치
- imported skeletal mesh `.bin` 또는 manifest entry를 drag/drop하면 해당 mesh를 바로 배치

검증 지점:

- `ReleaseBuild.bat`
- 에디터에서 임포트 전 FBX 내부 skeletal mesh 선택 불가 확인
- Contents drawer에서 미임포트 FBX 더블클릭 시 import 후 viewer open 확인
- Contents drawer에서 이미 import된 FBX 더블클릭 시 재임포트 없이 viewer open 확인
- Contents drawer에서 FBX 원본 viewport drag/drop 시 import 및 배치 수행 확인
- 임포트 후 component property combo에 imported mesh만 표시 확인
- 배치/저장/재로드 시 같은 mesh asset path 유지 확인

### 10. 캐시 삭제와 패키징 경로 보정

변경 후보:

- `FResourceManager::DeleteAllCacheFiles`
- `GamePackager`
- asset cook 경로 정책

작업:

- sibling `.bin`, skeleton `.bin`, animation `.bin`, manifest 삭제 규칙 추가
- 패키징에서 skeletal mesh, skeleton, animation 종속 파일 수집
- 상대경로 참조가 패키징 후에도 유지되는지 확인

검증 지점:

- `ReleaseBuild.bat`
- 에디터 cache delete 기능 수행 후 신규 캐시가 모두 삭제되는지 확인
- 샘플 패키징이 있다면 package build 수행
- 패키징 결과물에서 skeleton/mesh/animation 로드 확인

### 11. 회귀 검증

필수 시나리오:

- 기존 단일 skeletal mesh FBX 로드
- static mesh FBX 로드
- OBJ/static mesh 로드
- skeletal mesh viewer bone tree/socket 편집/저장
- scene 저장 후 재오픈 시 skeletal mesh component asset path 복원
- animation clip import/load
- 캐시 삭제 후 재임포트

권장 시나리오:

- 하나의 FBX에 skeleton 1개 + skeletal mesh 여러 개
- 하나의 FBX에 skeleton 여러 개
- 하나의 FBX에 animation stack 여러 개
- 같은 skeleton을 공유하는 mesh 병합
- material slot 이름 중복/빈 이름/비ASCII 이름
- bone count 256 초과 FBX

최종 검증 지점:

- `GenerateProjectFiles.bat`
- `ReleaseBuild.bat`
- 에디터 수동 smoke test
- 샘플 asset import 로그와 생성 파일 목록 캡처

## 잠재 우려사항

- 현재 vertex bone index가 `uint8`로 보이며 256개 이상 bone은 기존에도 제한이 있다. 이번 작업에서는 제한을 유지한다.
- socket은 현재 `FSkeletalMesh`에 저장된다. skeleton 공용 에셋으로 옮기면 기존 socket editor/save 흐름이 skeleton 저장 흐름으로 이동해야 한다.
- `FAnimationClip::SkeletonSourcePath`는 있지만 animation 런타임이 실제 skeleton compatibility를 강제하는 흐름은 약해 보인다. 로드 시 검증만 할지, 재생 시에도 막을지 결정이 필요하다.
- "임포트"는 Contents drawer에서 FBX 더블클릭 또는 FBX viewport drag/drop 시 수행하는 명시 동작으로 확정한다. 이미 import된 FBX 더블클릭은 재임포트가 아니라 기존 imported skeletal mesh viewer open으로 처리한다.
- sibling `.bin` 정책은 FBX 폴더가 쓰기 불가능한 위치일 때 실패할 수 있다. 현재 Asset 폴더 기준 운용이면 문제는 작지만 오류 처리 정책이 필요하다.
- 다중 skeleton FBX에서 animation stack 하나가 여러 skeleton에 모두 적용 가능한 경우는 고려하지 않는다. 구현 편의상 하나의 target skeleton만 연결한다.
- material slot 기준 병합 시 mesh별 transform, geometry transform, bind pose 기준이 다르면 현재 vertex bake 정책과 충돌할 수 있다. 병합 전 샘플 FBX로 검증이 필요하다.

## 답변 반영 완료

- 명시적 import만 허용. Contents drawer에서 미임포트 FBX 더블클릭 시 import 후 viewer open.
- 이미 import된 FBX 더블클릭은 재임포트 없이 기존 imported skeletal mesh viewer open.
- 기존에 FBX 파일 viewport drag/drop 배치가 지원되므로, FBX drag/drop도 명시적 import 후 imported skeletal mesh 일괄 배치로 유지.
- socket은 skeleton 공용 에셋에 저장.
- 단일 FBX 내 여러 skeletal mesh는 character part로 보고 material slot 기준 병합.
- `skeletonName`은 skeleton root FbxNode 이름 기준.
- animation stack이 여러 skeleton에 적용 가능한 경우는 고려하지 않음.
- bone index 제한은 현행 유지.

## 남은 확인 질문

1. import manifest 파일명은 `{fbxStem}_import.json`으로 두어도 될까요? 같은 폴더에 sibling `.bin`을 두는 정책과 맞춰 가장 단순합니다.
