#include "pch.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "ParticleEmitterInstance.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"

FParticleEmitterInstance::FParticleEmitterInstance()
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
	, CurrentLODLevelIndex(-1)
{
}

FParticleEmitterInstance::~FParticleEmitterInstance()
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

void FParticleEmitterInstance::InitParticles(int32 InMaxParticles)
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

void FParticleEmitterInstance::Initialize(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, int32 InLODIndex, int32 InMaxActiveParticles)
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

    // @TODO: LOD 구현은 후순위. 구현 전까지 SetLODLevel 호출하지 말 것. 
    //SetLODLevel(InLODIndex);
    SetLODLevel(0);
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

uint32 FParticleEmitterInstance::CalculateParticleStride() const
{
    uint32 ParticleSize = sizeof(FBaseParticle);

    if (CurrentLODLevel)
    {
        ParticleSize += CurrentLODLevel->GetRequiredBytes();
    }

    return AlignUp(ParticleSize, ParticleStrideAlignment);
}

void FParticleEmitterInstance::ReallocateParticleData(uint32 NewStride)
{
    if (NewStride == ParticleStride)
    {
        return;
    }

    ParticleStride = NewStride;

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

    ActiveParticles = 0;

    if (MaxActiveParticles > 0)
    {
        InitParticles(MaxActiveParticles);
    }
}

void FParticleEmitterInstance::Tick(float DeltaTime)
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

void FParticleEmitterInstance::SpawnParticles(float DeltaTime)
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

	// 이번 프레임에 스폰할 파티클 수 계산
	const float Desired = SpawnFraction + SpawnRate * DeltaTime; // 이번 프레임에 스폰할 파티클 수 (이전 프레임 소수점 단위 이월받음 + 소수점 포함)
	int32 SpawnCount = static_cast<int32>(std::floor(Desired)); // 정수 부분만 남겨 실제 스폰할 파티클 수
	SpawnFraction = Desired - SpawnCount; // 실수 부분은 다음 프레임으로 이월

    // Capacity 넘길 수 없도록 제한
    const int32 CapacityLeft = MaxActiveParticles - ActiveParticles;
    SpawnCount = FMath::Min(SpawnCount, CapacityLeft);

    if (SpawnCount <= 0)
    {
        return;
    }

    // Spawn 단계에서 실행할 모듈 얻어오기
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

void FParticleEmitterInstance::RunUpdateModules(float DeltaTime)
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

void FParticleEmitterInstance::RunFinalUpdateModules(float DeltaTime)
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

void FParticleEmitterInstance::Sort(EParticleSortMode SortMode, const FVector* ViewLocation)
{
    if (SortMode == EParticleSortMode::None || ActiveParticles <= 1)
        return;

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

    default:
        break;
    }
}

void FParticleEmitterInstance::KillDeadParticles()
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

void FParticleEmitterInstance::KillParticle(int32 ActiveIndex)
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

void FParticleEmitterInstance::Reset()
{
    ActiveParticles = 0;
    SpawnFraction = 0.0f;
    SecondsSinceCreation = 0.0f;
}

// @TODO: LOD 구현은 후순위. 구현 전까지 SetLODLevel 호출하지 말 것. 
void FParticleEmitterInstance::SetLODLevel(int32 LODIndex)
{
    if (!EmitterTemplate || LODIndex < 0)
    {
        if (CurrentLODLevel)
        {
            CurrentLODLevel = nullptr;
            CurrentLODLevelIndex = -1;
            const uint32 BaseStride = AlignUp(sizeof(FBaseParticle), ParticleStrideAlignment);
            ReallocateParticleData(BaseStride);
        }
        return;
    }

    if (CurrentLODLevelIndex == LODIndex && CurrentLODLevel != nullptr)
    {
        return;
    }

    UParticleLODLevel* LOD = EmitterTemplate->GetLODLevel(LODIndex);
    if (!LOD)
    {
        return;
    }

    CurrentLODLevel = LOD;
    CurrentLODLevelIndex = LODIndex;

    const uint32 NewStride = CalculateParticleStride();
    ReallocateParticleData(NewStride);
}

FBaseParticle* FParticleEmitterInstance::GetParticle(int32 ActiveIndex)
{
    if (ActiveIndex < 0 || ActiveIndex >= ActiveParticles)
        return nullptr;
    return reinterpret_cast<FBaseParticle*>(ParticleData + ParticleIndices[ActiveIndex] * ParticleStride);
}

const FBaseParticle* FParticleEmitterInstance::GetParticle(int32 ActiveIndex) const
{
    if (ActiveIndex < 0 || ActiveIndex >= ActiveParticles)
        return nullptr;
    return reinterpret_cast<const FBaseParticle*>(ParticleData + ParticleIndices[ActiveIndex] * ParticleStride);
}
