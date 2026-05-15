# Animation Clip Import Cookbook

이 문서는 후속 작업자가 애니메이션 import 코드를 바로 집어 쓸 수 있게 만든 짧은 지도다.

## 애니메이션을 로드하고 싶다

`FResourceManager::LoadAnimationClip(Path)`를 호출한다.

이 함수는 메모리 캐시를 먼저 보고, binary cache가 유효하면 `.bin`을 읽고, 아니면 FBX에서 다시 import한 뒤 cache를 저장한다. 실제 흐름은 `FAnimationClipLoadService::Load()`가 담당한다.

## FBX에서 animation을 import하고 싶다

`FFbxImporter::LoadAnimations(Path, Options)`를 사용한다.

이 함수는 FBX의 `AnimStack`을 `FAnimationClip`으로 바꾼다. bone track 수집은 `ExtractBoneAnimationTracks()`가 담당하며, FBX SDK의 `EvaluateLocalTransform()`으로 clip frame rate 기준 샘플을 만든다. FBX의 원본 `FbxAnimCurve` key/tangent metadata는 엔진 데이터로 복사하지 않는다.

## 결과 데이터는 어디에 있나

핵심 타입은 `AnimationTypes.h`에 있다.

`FAnimationClip`은 clip 단위 데이터이고, `FBoneAnimationTrack`은 bone 하나의 샘플링된 TRS curve를 담는다. 각 축 curve는 `FAnimationFloatCurve`, 각 key는 시간/값만 가진 `FAnimationCurveKey`다.

## runtime asset wrapper가 필요하다

`UAnimationClipAsset`을 쓴다.

`UAnimationAssetBase`는 playback용 베이스이고, `UAnimationClipAsset`은 imported `FAnimationClip`을 소유하는 구현체다. 데이터는 `GetClipData()`, source FBX 경로는 `GetSourcePath()`, 길이는 `GetPlayLength()`로 본다.

## binary cache를 저장하거나 읽고 싶다

`FAnimationClipSerializer`를 쓴다.

`SaveAnimationClip()`은 `FAnimationClip`을 binary로 저장하고, `LoadAnimationClip()`은 다시 읽는다. cache freshness 확인용 header만 읽을 때는 `ReadAnimationClipHeader()`를 쓴다.

## animation cache 경로가 필요하다

`FAssetPathPolicy::MakeWritableAnimationClipCacheBinaryPath(SourcePath)`를 쓴다.

현재 cache 위치는 `Asset/Animation/Bin/*.bin`이다. `.animclip` asset path 판별은 `IsAnimationClipAssetPath()`에 있다.

## asset discovery에 animation clip을 포함하고 싶다

`FAssetQueryService::GetAnimationClipPaths()`와 `FResourceManager::GetAnimationClipPaths()`를 쓴다.

현재 discovery는 `Asset/Animation/*.animclip` 기준이다. FBX source discovery와 serialized `.animclip` 정책은 아직 분리 정리가 필요하다.

## import 결과가 이상한지 확인하고 싶다

`ValidateAnimationClip()`을 보면 된다.

clip time range, duplicate bone track, empty bone name, non-finite key time/value를 검사한다.

## 지금 구현 범위

현재 완성된 범위는 `FBX AnimStack -> SDK-sampled bone name 기반 FAnimationClip -> binary cache -> UAnimationClipAsset`이다.

아직 없는 것은 runtime pose evaluation, skinning 반영, editor preview playback, retargeting, compression, shape key evaluation이다.
