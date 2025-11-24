#pragma once

#include "Object.h"
#include "ParticleEmitter.h"
#include "UParticleSystem.generated.h"

/** @brief: 파티클의 LOD 계산 방식을 정의합니다.*/
enum ParticleSystemLODMethod
{
    Automatic,        // 일정 주기마다(LODDistanceCheckTime) 거리 기반으로 LOD를 자동으로 갱신
    DirectSet,        // 게임 코드에서 LODIndex를 직접 세팅
    ActivateAutomatic // 처음 활성화될 때 한 번 거리 기반으로 LOD를 정하고, 그 이후에는 코드로 직접 바꾸지 않는 한 고정
};

// 파티클 시스템 에셋 (최상위)
// - 여러 이미터를 포함하는 컨테이너
// - 에디터에서 편집 가능, 런타임에 여러 인스턴스 공유
UCLASS(DisplayName="파티클 시스템", Description="파티클 이미터들을 포함하는 에셋입니다")
class UParticleSystem : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleSystem();

    // 이미터 배열
    // - 각 이미터는 독립적인 파티클 그룹 (예: 불꽃, 연기, 불똥)
    UPROPERTY(EditAnywhere, Category="Emitters")
    TArray<UParticleEmitter*> Emitters;

    // 시스템 이름
    UPROPERTY(EditAnywhere, Category="System")
    FString SystemName;

    // 전체 시스템 지속 시간 (0 = 무한)
    UPROPERTY(EditAnywhere, Category="System")
    float SystemDuration;

    // 시스템 반복 횟수 (0 = 무한)
    UPROPERTY(EditAnywhere, Category="System")
    int32 SystemLoops;

    // 자동 재생 여부
    UPROPERTY(EditAnywhere, Category="System")
    bool bAutoActivate;

    // 이미터 접근
    UParticleEmitter* GetEmitter(int32 Index) const;
    void AddEmitter(UParticleEmitter* Emitter);
    void RemoveEmitter(UParticleEmitter* Emitter);
    int32 GetEmitterCount() const { return Emitters.Num(); }
};
