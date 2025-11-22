#pragma once

#include "Vector.h"
#include "Color.h"

// 파티클 이미터 타입
enum class EDynamicEmitterType : uint8
{
    Sprite,      // 스프라이트 파티클 (빌보드)
    Mesh,        // 메시 파티클 (3D 메시)
    Unknown
};

// 파티클 정렬 모드
enum class EParticleSortMode : uint8
{
    None,                    // 정렬 안 함
    ViewDistanceDepth,       // 카메라 거리 기준
    AgeOldestFirst,         // 나이 많은 순
    AgeNewestFirst          // 나이 적은 순
};

// 파티클 분포 타입 (위치/속도 분포)
enum class EDistributionType : uint8
{
    Constant,       // 고정값
    Uniform,        // 균등 분포
    ConstantCurve,  // 시간별 커브 (선택)
    Particle        // 파티클별 랜덤
};

// 전방 선언
class UParticleEmitter;
class UParticleLODLevel;
class UParticleSystemComponent;

// 파티클 데이터 구조
// - 연속된 메모리 블록에 저장되는 파티클 데이터
// - Stride 기반 인덱싱으로 동적 페이로드 지원
struct FBaseParticle
{
    // Core State (48 bytes)
    FVector Location;            // 12 bytes - 현재 위치
    FVector OldLocation;         // 12 bytes - 이전 위치 (트레일/충돌 용)
    FVector BaseVelocity;        // 12 bytes - 기본 속도
    FVector Velocity;            // 12 bytes - 현재 속도

    // Size/Rotation (32 bytes)
    FVector Size;                // 12 bytes - 현재 크기
    FVector BaseSize;            // 12 bytes - 초기 크기
    float Rotation;              // 4 bytes - 현재 회전
    float BaseRotationRate;      // 4 bytes - 초기 회전 속도

    // Color (32 bytes)
    FLinearColor Color;          // 16 bytes - 현재 컬러
    FLinearColor BaseColor;      // 16 bytes - 초기 컬러

    // Lifecycle (12 bytes)
    float RelativeTime;          // 0.0 ~ 1.0 (생명 비율)
    float OneOverMaxLifetime;    // 1.0 / MaxLifetime (최적화)
    int32 Flags;                 // 파티클 플래그

    // Total: ~128 bytes (정렬 포함)

    FBaseParticle()
        : Location(FVector::Zero())
        , OldLocation(FVector::Zero())
        , BaseVelocity(FVector::Zero())
        , Velocity(FVector::Zero())
        , Size(FVector::One())
        , BaseSize(FVector::One())
        , Rotation(0.0f)
        , BaseRotationRate(0.0f)
        , Color(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        , BaseColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        , RelativeTime(0.0f)
        , OneOverMaxLifetime(0.0f)
        , Flags(0)
    {
    }
};

// 파티클 이미터 인스턴스 (런타임)
// - 실제 파티클 시뮬레이션을 담당하는 비-UObject 구조체
// - 메모리 풀 관리 및 파티클 생명주기 제어
struct FParticleEmitterInstance
{
    UParticleEmitter* EmitterTemplate;       // 에셋 참조
    UParticleLODLevel* CurrentLODLevel;      // 현재 LOD
    UParticleSystemComponent* Component;     // 소유 컴포넌트

    // 메모리 관리
    uint8* ParticleData;                           // 연속된 파티클 메모리 블록
    int32* ParticleIndices;                        // 활성 파티클 인덱스 배열
    uint32 ParticleStride;                         // sizeof(FBaseParticle) + 추가 페이로드
    int32 ActiveParticles;                         // 현재 활성 파티클 수
    int32 MaxActiveParticles;                      // 최대 파티클 수

    // 스폰 제어
    float SpawnFraction;                           // 누적 스폰 잔량
    float SecondsSinceCreation;                    // 생성 후 경과 시간

    FParticleEmitterInstance()
        : EmitterTemplate(nullptr)
        , CurrentLODLevel(nullptr)
        , Component(nullptr)
        , ParticleData(nullptr)
        , ParticleIndices(nullptr)
        , ParticleStride(sizeof(FBaseParticle))
        , ActiveParticles(0)
        , MaxActiveParticles(0)
        , SpawnFraction(0.0f)
        , SecondsSinceCreation(0.0f)
    {
    }

    ~FParticleEmitterInstance()
    {
        if (ParticleData)
        {
            delete[] ParticleData;
            ParticleData = nullptr;
        }
        if (ParticleIndices)
        {
            delete[] ParticleIndices;
            ParticleIndices = nullptr;
        }
    }

    // Stride 기반 파티클 접근
    FBaseParticle* GetParticle(int32 Index)
    {
        if (Index < 0 || Index >= ActiveParticles)
            return nullptr;
        return reinterpret_cast<FBaseParticle*>(ParticleData + ParticleIndices[Index] * ParticleStride);
    }

    const FBaseParticle* GetParticle(int32 Index) const
    {
        if (Index < 0 || Index >= ActiveParticles)
            return nullptr;
        return reinterpret_cast<const FBaseParticle*>(ParticleData + ParticleIndices[Index] * ParticleStride);
    }

    // 메모리 할당
    void InitParticles(int32 InMaxParticles)
    {
        MaxActiveParticles = InMaxParticles;
        ParticleData = new uint8[MaxActiveParticles * ParticleStride];
        ParticleIndices = new int32[MaxActiveParticles];
        for (int32 i = 0; i < MaxActiveParticles; ++i)
        {
            ParticleIndices[i] = i;
        }
        ActiveParticles = 0;
    }

    // 파티클 정렬
    void Sort(EParticleSortMode SortMode = EParticleSortMode::None, const FVector* ViewLocation = nullptr)
    {
        if (SortMode == EParticleSortMode::None || ActiveParticles <= 1)
            return;

        // 정렬 모드에 따라 ParticleIndices 재정렬
        switch (SortMode)
        {
        case EParticleSortMode::ViewDistanceDepth:
            if (ViewLocation)
            {
                std::sort(ParticleIndices, ParticleIndices + ActiveParticles,
                    [this, ViewLocation](int32 A, int32 B) {
                        FBaseParticle* ParticleA = reinterpret_cast<FBaseParticle*>(ParticleData + A * ParticleStride);
                        FBaseParticle* ParticleB = reinterpret_cast<FBaseParticle*>(ParticleData + B * ParticleStride);
                        float DistA = (ParticleA->Location - *ViewLocation).SizeSquared();
                        float DistB = (ParticleB->Location - *ViewLocation).SizeSquared();
                        return DistA > DistB; // 먼 것부터 (뒤에서 앞으로)
                    });
            }
            break;

        case EParticleSortMode::AgeOldestFirst:
            std::sort(ParticleIndices, ParticleIndices + ActiveParticles,
                [this](int32 A, int32 B) {
                    FBaseParticle* ParticleA = reinterpret_cast<FBaseParticle*>(ParticleData + A * ParticleStride);
                    FBaseParticle* ParticleB = reinterpret_cast<FBaseParticle*>(ParticleData + B * ParticleStride);
                    return ParticleA->RelativeTime > ParticleB->RelativeTime;
                });
            break;

        case EParticleSortMode::AgeNewestFirst:
            std::sort(ParticleIndices, ParticleIndices + ActiveParticles,
                [this](int32 A, int32 B) {
                    FBaseParticle* ParticleA = reinterpret_cast<FBaseParticle*>(ParticleData + A * ParticleStride);
                    FBaseParticle* ParticleB = reinterpret_cast<FBaseParticle*>(ParticleData + B * ParticleStride);
                    return ParticleA->RelativeTime < ParticleB->RelativeTime;
                });
            break;
        }
    }
};
