#include "pch.h"
#include "ParticleModuleRequired.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"

UParticleModuleRequired::UParticleModuleRequired()
    : SpawnRate(10.0f)
    , EmitterDuration(0.0f)
    , EmitterLoops(0)
    , Material(nullptr)
    , EmitterOrigin()
    , EmitterRotation(0.0f)
    , bUseLocalSpace(false)
{
    // Required 모듈은 Spawn 단계에서만 실행
    bSpawnModule = true;
    bUpdateModule = false;
    bFinalUpdateModule = false;

    ModuleName = "Required";
}

void UParticleModuleRequired::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    // MUNDI_SPAWN_INIT 매크로 사용
    MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

    // 필수 모듈에서는 기본적인 초기화만 수행
    // 실제 Lifetime, Velocity 등은 각각의 전용 모듈에서 설정
}
