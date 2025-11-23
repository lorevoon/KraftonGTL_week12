#pragma once

#include <cmath>
#include <cstring>
#include "Vector.h"
#include "Color.h"
#include "ParticleEmitter.h"
#include "ParticleModule.h"

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
class FSceneView;
struct FDynamicEmitterDataBase;

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
    static constexpr uint32 ParticleStrideAlignment = 16u; // 파티클 메모리 정렬 단위

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

    /**
	 * @brief 주어진 값과 정렬 단위에 맞춰 값을 올림 정렬합니다.
	 * @note: Alignment는 2의 거듭제곱이어야 합니다.
     */
    static uint32 AlignUp(uint32 Value, uint32 Alignment)
    {
        // Alignment를 이진수로 표현하면 한 비트만 1인 형태 -> Mask는 그 아래 값이 전부 1, 나머지는 0
        const uint32 Mask = Alignment - 1u;
        // Value에 Mask(=Alignment -1)을 더하면
        // 1. value가 이미 정렬된 경우 -> 다음 정렬 넘어가기 직전 값이 됨
		// 2. value가 정렬되지 않은 경우 -> 다음 정렬 경계값 이상의 값이 됨
        // ~Mask와 AND연산하면 정렬 경계값으로 clipping됨.
        return (Value + Mask) & ~Mask;
    }

    // Stride 기반 파티클 접근
    FBaseParticle* GetParticle(int32 ActiveIndex)
    {
        if (ActiveIndex < 0 || ActiveIndex >= ActiveParticles)
            return nullptr;
        return reinterpret_cast<FBaseParticle*>(ParticleData + ParticleIndices[ActiveIndex] * ParticleStride);
    }

    const FBaseParticle* GetParticle(int32 ActiveIndex) const
    {
        if (ActiveIndex < 0 || ActiveIndex >= ActiveParticles)
            return nullptr;
        return reinterpret_cast<const FBaseParticle*>(ParticleData + ParticleIndices[ActiveIndex] * ParticleStride);
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
 
    /**
	 * @brief 상위 초기화를 수행합니다. (템플릿/컴포넌트/LOD와 메모리 풀 준비)
	 * @param InTemplate 이미터 템플릿
	 * @param InComponent 소유 파티클 시스템 컴포넌트
	 * @param InLODIndex 초기 LOD 인덱스
	 * @param InMaxActiveParticles 최대 활성 파티클 수 (0이하 값을 주면 템플릿 설정을 사용합니다)
     */
    void Initialize(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, int32 InLODIndex, int32 InMaxActiveParticles)
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

        EmitterTemplate = InTemplate;
        Component = InComponent;
        SetLODLevel(InLODIndex);
        ParticleStride = CalculateParticleStride(); // 페이로드 요구량 + 정렬 반영

		// 파티클 최댓값: 지정값 우선, 없으면 이미터 템플릿 기준
        const int32 RequestedMax = (InMaxActiveParticles > 0) ? InMaxActiveParticles :
            (EmitterTemplate ? EmitterTemplate->MaxParticleCount : 0);

        if (RequestedMax > 0)
        {
            InitParticles(RequestedMax);
        }
        else
        {
            MaxActiveParticles = 0;
            ActiveParticles = 0;
        }

        SpawnFraction = 0.0f;
        SecondsSinceCreation = 0.0f;
    }

    /** @brief: 모듈 요구 바이트를 합산해 정렬(align)까지 고려한 Stride를 계산합니다. */
    uint32 CalculateParticleStride() const
    {
        uint32 ParticleSize = sizeof(FBaseParticle);

        if (CurrentLODLevel)
        {
            ParticleSize += CurrentLODLevel->GetRequiredBytes();
        }

        return AlignUp(ParticleSize, ParticleStrideAlignment);
    }

    /** @brief 한 프레임 틱 업데이트를 수행합니다. (스폰 → 업데이트 → 파이널 업데이트 → Kill) */
    void Tick(float DeltaTime)
    {
        if (!CurrentLODLevel || MaxActiveParticles == 0)
        {
            return;
        }

        SpawnParticles(DeltaTime);
        RunUpdateModules(DeltaTime);
        RunFinalUpdateModules(DeltaTime);
        KillDeadParticles();

        SecondsSinceCreation += DeltaTime;
    }

    // SpawnRate 기반 파티클 생성
    void SpawnParticles(float DeltaTime)
    {
        if (!CurrentLODLevel || !CurrentLODLevel->RequiredModule)
        {
            return;
        }

        if (ActiveParticles >= MaxActiveParticles)
        {
            return;
        }

        const float SpawnRate = CurrentLODLevel->RequiredModule->SpawnRate;
        if (SpawnRate <= 0.0f)
        {
            return;
        }

        const float Desired = SpawnFraction + SpawnRate * DeltaTime;
        int32 SpawnCount = static_cast<int32>(std::floor(Desired));
        SpawnFraction = Desired - SpawnCount;

        const int32 CapacityLeft = MaxActiveParticles - ActiveParticles;
        SpawnCount = FMath::Min(SpawnCount, CapacityLeft);

        if (SpawnCount <= 0)
        {
            return;
        }

        const TArray<UParticleModule*> SpawnModules = CurrentLODLevel->GetSpawnModules();

        for (int32 i = 0; i < SpawnCount; ++i)
        {
            const int32 NewActiveIndex = ActiveParticles;

            if (NewActiveIndex >= MaxActiveParticles)
            {
                break;
            }

            const int32 DataIndex = ParticleIndices[NewActiveIndex];
            FBaseParticle* Particle = reinterpret_cast<FBaseParticle*>(ParticleData + DataIndex * ParticleStride);
            std::memset(Particle, 0, ParticleStride);
            Particle->RelativeTime = 0.0f;
            Particle->OneOverMaxLifetime = 0.0f;
            Particle->OldLocation = Particle->Location;

            for (UParticleModule* Module : SpawnModules)
            {
                if (Module && Module->bEnabled)
                {
                    Module->Spawn(this, NewActiveIndex, SecondsSinceCreation, Particle);
                }
            }

            Particle->OldLocation = Particle->Location;
            ActiveParticles++;
        }
    }

    // Update 단계 모듈 실행
    void RunUpdateModules(float DeltaTime)
    {
        if (!CurrentLODLevel)
        {
            return;
        }

        for (int32 i = 0; i < ActiveParticles; ++i)
        {
            FBaseParticle* Particle = GetParticle(i);
            if (Particle && Particle->OneOverMaxLifetime > 0.0f)
            {
                Particle->RelativeTime += DeltaTime * Particle->OneOverMaxLifetime;
            }
        }

        const TArray<UParticleModule*> UpdateModules = CurrentLODLevel->GetUpdateModules();
        for (UParticleModule* Module : UpdateModules)
        {
            if (Module && Module->bEnabled)
            {
                Module->Update(this, 0, DeltaTime);
            }
        }
    }

    // FinalUpdate 단계 모듈 실행
    void RunFinalUpdateModules(float DeltaTime)
    {
        if (!CurrentLODLevel)
        {
            return;
        }

        const TArray<UParticleModule*> FinalModules = CurrentLODLevel->GetFinalUpdateModules();
        for (UParticleModule* Module : FinalModules)
        {
            if (Module && Module->bEnabled)
            {
                Module->FinalUpdate(this, 0, DeltaTime);
            }
        }
    }

    // 사망 파티클 정리
    void KillDeadParticles()
    {
        for (int32 i = ActiveParticles - 1; i >= 0; --i)
        {
            const FBaseParticle* Particle = GetParticle(i);
            if (Particle && Particle->RelativeTime >= 1.0f)
            {
                KillParticle(i);
            }
        }
    }

    // 특정 활성 인덱스 파티클 제거
    void KillParticle(int32 ActiveIndex)
    {
        if (ActiveIndex < 0 || ActiveIndex >= ActiveParticles)
        {
            return;
        }

        const int32 LastActive = ActiveParticles - 1;
        if (ActiveIndex != LastActive)
        {
            ParticleIndices[ActiveIndex] = ParticleIndices[LastActive];
        }
        ActiveParticles = LastActive;
    }

    // 상태 리셋(메모리 유지)
    void Reset()
    {
        ActiveParticles = 0;
        SpawnFraction = 0.0f;
        SecondsSinceCreation = 0.0f;
    }

    // LOD 전환
    void SetLODLevel(int32 LODIndex)
    {
        if (!EmitterTemplate || LODIndex < 0)
        {
            CurrentLODLevel = nullptr;
            return;
        }

        UParticleLODLevel* LOD = EmitterTemplate->GetLODLevel(LODIndex);
        CurrentLODLevel = LOD;
    }
};
