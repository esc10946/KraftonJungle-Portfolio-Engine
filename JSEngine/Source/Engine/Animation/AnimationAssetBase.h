#pragma once

#include "Object/Object.h"

// 애니메이션 재생 계층에서 공통으로 바라보는 API 성격의 계층.
// FBX에서 추출한 커브 데이터 등은 직접 가지고 있지 않고, 자식 클래스가 소유한다.
class UAnimationAssetBase : public UObject
{
public:
    DECLARE_CLASS(UAnimationAssetBase, UObject)

    virtual float GetPlayLength() const { return 0.0f; }
};
