#pragma once

#include "ParticleTypes.h"
#include "ParticleEmitterInstance.h"

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

// ============================================================
// 동적 이미터 렌더링 데이터 (다형성 구조)
// ============================================================

// 전방 선언
class FSceneView;
class UMaterialInterface;
class D3D11RHI;
class UParticleDynamicBuffers;
struct ID3D11DeviceContext;

// Sprite 파티클 인스턴스 데이터
struct FParticleSpriteInstance
{
    // 셰이더와 메모리 레이아웃을 일치시키기 위해 명시적으로 float로 선언
    // 16바이트 단위로 정렬 (셰이더 상수 버퍼 규칙과 FVector4 alignas(16) 대응)
    float WorldPositionX, WorldPositionY, WorldPositionZ, Padding0;  // 16 bytes
    float SizeX, SizeY, Rotation, Padding1;                          // 16 bytes
    float ColorR, ColorG, ColorB, ColorA;                            // 16 bytesㅉ

    FParticleSpriteInstance()
        : WorldPositionX(0.0f), WorldPositionY(0.0f), WorldPositionZ(0.0f), Padding0(0.0f)
        , SizeX(1.0f), SizeY(1.0f), Rotation(0.0f), Padding1(0.0f)
        , ColorR(1.0f), ColorG(1.0f), ColorB(1.0f), ColorA(1.0f)
    {
    }
};

// 추상 베이스 클래스
struct FDynamicEmitterDataBase
{
    FParticleEmitterInstance* Source;  // 원본 파티클 데이터

    FDynamicEmitterDataBase(FParticleEmitterInstance* InSource)
        : Source(InSource)
    {
    }

    virtual ~FDynamicEmitterDataBase() {}

    // 버퍼 업데이트 (각 타입별 구현)
    virtual void UpdateRenderData(FSceneView* View) = 0;

    // 렌더링 (각 타입별 구현)
    virtual void Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material, UParticleDynamicBuffers* BufferPool) = 0;
};

// Sprite 파티클 렌더링 데이터
struct FDynamicSpriteEmitterData : public FDynamicEmitterDataBase
{
    TArray<FParticleSpriteInstance> Instances;  // 각 파티클의 인스턴스 데이터

    FDynamicSpriteEmitterData(FParticleEmitterInstance* InSource)
        : FDynamicEmitterDataBase(InSource)
    {
    }

    virtual void UpdateRenderData(FSceneView* View) override;
    virtual void Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material, UParticleDynamicBuffers* BufferPool) override;

private:
    // 파티클 인스턴스 데이터 생성
    void BuildSpriteInstances(FSceneView* View);
};

// Mesh 파티클 렌더링 데이터
struct FDynamicMeshEmitterData : public FDynamicEmitterDataBase
{
    TArray<FMatrix> InstanceTransforms;  // 각 파티클의 Transform
    TArray<FVector4> InstanceColors;      // 각 파티클의 Color

    FDynamicMeshEmitterData(FParticleEmitterInstance* InSource)
        : FDynamicEmitterDataBase(InSource)
    {
    }

    virtual void UpdateRenderData(FSceneView* View) override;
    virtual void Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material, UParticleDynamicBuffers* BufferPool) override;

private:
    // 각 파티클의 인스턴스 데이터 생성
    void BuildMeshInstances();
};
