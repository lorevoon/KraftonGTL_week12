#pragma once

#include "ParticleModule.h"
#include "UParticleModuleTypeDataBase.generated.h"

// TypeData 모듈 베이스 클래스
// - 파티클 이미터 타입을 결정하는 특수 모듈
// - Spawn/Update 호출되지 않음 (타입 정의만)
// - TypeData가 없으면 Sprite (기본 타입)
UCLASS(Abstract, DisplayName="타입 데이터 베이스", Description="파티클 이미터의 타입을 결정하는 베이스 클래스입니다")
class UParticleModuleTypeDataBase : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    
    UParticleModuleTypeDataBase() {}
    virtual ~UParticleModuleTypeDataBase() {}

    // TypeData 모듈은 Spawn/Update 호출 안 됨
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override {}
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override {}
};
