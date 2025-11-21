#pragma once

#include "ParticleTypes.h"

// 파티클 헬퍼 매크로
// - 반복적인 파티클 접근 코드를 간소화
// - 가독성 및 유지보수성 향상

// ============================================================
// 매크로 1: MUNDI_DECLARE_PARTICLE_PTR
// ============================================================
// 파티클 포인터 선언 및 초기화를 간소화
//
// 사용 예:
// void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
// {
//     MUNDI_DECLARE_PARTICLE_PTR(Owner, Offset);
//     Particle->Velocity += FVector(0, 0, -980) * DeltaTime;
// }
//
#define MUNDI_DECLARE_PARTICLE_PTR(OwnerVar, OffsetVar) \
    FBaseParticle* Particle = (FBaseParticle*)((uint8*)(OwnerVar)->ParticleData + (OwnerVar)->ParticleStride * (OwnerVar)->ParticleIndices[OffsetVar]);

// ============================================================
// 매크로 2: MUNDI_BEGIN_UPDATE_LOOP / MUNDI_END_UPDATE_LOOP
// ============================================================
// Update 함수에서 모든 활성 파티클을 순회하는 루프를 간소화
//
// 사용 예:
// void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
// {
//     MUNDI_BEGIN_UPDATE_LOOP
//     {
//         Particle->Velocity += Acceleration * DeltaTime;
//         Particle->Location += Particle->Velocity * DeltaTime;
//     }
//     MUNDI_END_UPDATE_LOOP;
// }
//
#define MUNDI_BEGIN_UPDATE_LOOP \
    for (int32 i = 0; i < Owner->ActiveParticles; ++i) \
    { \
        FBaseParticle* Particle = Owner->GetParticle(i);

#define MUNDI_END_UPDATE_LOOP \
    }

// ============================================================
// 매크로 3: MUNDI_SPAWN_INIT
// ============================================================
// Spawn 함수에서 파티클 초기화를 간소화
//
// 사용 예:
// void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
// {
//     MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);
//     Particle->BaseVelocity = InitialVelocity;
//     Particle->Velocity = InitialVelocity;
// }
//
#define MUNDI_SPAWN_INIT(OwnerVar, OffsetVar, ParticleBaseVar) \
    FBaseParticle* Particle = (FBaseParticle*)((uint8*)(OwnerVar)->ParticleData + (OwnerVar)->ParticleStride * (OwnerVar)->ParticleIndices[OffsetVar]); \
    if (ParticleBaseVar) { *Particle = *ParticleBaseVar; }

// ============================================================
// 추가 유틸리티 함수
// ============================================================

// 파티클 RelativeTime 업데이트 헬퍼
inline void UpdateParticleRelativeTime(FBaseParticle* Particle, float DeltaTime)
{
    Particle->RelativeTime += DeltaTime * Particle->OneOverMaxLifetime;
}

// 파티클 생존 여부 확인
inline bool IsParticleAlive(const FBaseParticle* Particle)
{
    return Particle->RelativeTime < 1.0f;
}

// 파티클 사망 처리
inline void KillParticle(FBaseParticle* Particle)
{
    Particle->RelativeTime = 1.0f;
}
