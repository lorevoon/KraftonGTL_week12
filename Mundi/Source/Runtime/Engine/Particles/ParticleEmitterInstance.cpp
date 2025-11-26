#include "pch.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "ParticleEmitterInstance.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "Modules/ParticleModuleRequired.h"
#include "Modules/ParticleModuleEventGenerator.h"
#include "ParticleSystemComponent.h"
#include "ParticleStatManager.h"

FParticleEmitterInstance::FParticleEmitterInstance()
    : EmitterTemplate(nullptr)
    , CurrentLODLevel(nullptr)
    , Component(nullptr)
    , ParticleData(nullptr)
    , ParticleIndices(nullptr)
    , ParticleStride(sizeof(FBaseParticle))
    , MaxActiveParticles(0)
    , ActiveParticles(0)
    , SpawnFraction(0.0f)
    , ParticleCounter(0)
    , EmitterTime(0.0f)
    , EmitterDuration(0.0f)
    , LoopCount(0)
    , CurrentLODLevelIndex(-1)
    , ModuleInstanceData(nullptr)
    , ModuleInstanceDataStride(0)
    , bEditorVisible(true)
    , EditorRenderMode(EEmitterRenderMode::Normal)
{
}

FParticleEmitterInstance::~FParticleEmitterInstance()
{
    ClearParticleData();
    ClearModuleInstanceData();
}

void FParticleEmitterInstance::InitParticles()
{
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
    ClearParticleData();

    EmitterTemplate = InTemplate;
    Component = InComponent;

    // 파티클 최댓값: 지정값 우선, 없으면 이미터 템플릿 기준
    MaxActiveParticles = (InMaxActiveParticles > 0) ? InMaxActiveParticles :
        (EmitterTemplate ? EmitterTemplate->MaxParticleCount : 0);

    // @TODO: LOD 구현은 후순위. 구현 전까지 SetLODLevel 호출하지 말 것.
    //SetLODLevel(InLODIndex);
    SetLODLevel(0);

    SpawnFraction = 0.0f;
    ParticleCounter = 0;
    EmitterTime = 0.0f;
    LoopCount = 0;

    // 모듈 인스턴스 데이터 초기화
    InitModuleInstanceData();
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

void FParticleEmitterInstance::ReallocateParticleData(uint32 NewStride, int32 NewMaxActiveParticles)
{
    ParticleStride = NewStride;
    MaxActiveParticles = NewMaxActiveParticles;

    ClearParticleData();

    if (ParticleStride > 0 && MaxActiveParticles > 0)
    {
        InitParticles();
    }
    else
    {
		UE_LOG("ERROR: FParticleEmitterInstance::ReallocateParticleData called with invalid parameters. (ParticleStride: %d, MaxActiveParticles: %d)", ParticleStride, MaxActiveParticles);
    }
}

void FParticleEmitterInstance::Tick(float DeltaTime)
{
    if (!CurrentLODLevel || MaxActiveParticles == 0)
    {
        return;
    }

    const bool bCanSpawn = CanSpawnThisFrame(DeltaTime);
    if (bCanSpawn)
    {
        SpawnParticles(DeltaTime);
    }

    RunUpdateModules(DeltaTime);
    RunFinalUpdateModules(DeltaTime);
    KillDeadParticles();

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
                Module->Spawn(this, NewActiveIndex, EmitterTime, Particle);
            }
		}

		Particle->OldLocation = Particle->Location;
		++ActiveParticles;
		++ParticleCounter;
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

    // Velocity integration: Apply velocity to location
    for (int32 i = 0; i < ActiveParticles; ++i)
    {
        FBaseParticle* Particle = GetParticle(i);
        if (Particle)
        {
            Particle->OldLocation = Particle->Location;
            Particle->Location = Particle->Location + (Particle->Velocity * DeltaTime);
        }
    }
}

void FParticleEmitterInstance::Sort(EParticleSortMode SortMode, const FVector* ViewLocation)
{
    if (SortMode == EParticleSortMode::None || ActiveParticles <= 1)
        return;

    // Local Space면 월드 좌표로 변환해서 비교
    const FVector WorldOffset = UseLocalSpace() ? GetComponentWorldLocation() : FVector::Zero();

    switch (SortMode)
    {
    case EParticleSortMode::ViewDistanceDepth:
        if (ViewLocation)
        {
            std::sort(ParticleIndices, ParticleIndices + ActiveParticles,
                [this, ViewLocation, WorldOffset](int32 A, int32 B) {
                    FBaseParticle* ParticleA = reinterpret_cast<FBaseParticle*>(ParticleData + A * ParticleStride);
                    FBaseParticle* ParticleB = reinterpret_cast<FBaseParticle*>(ParticleData + B * ParticleStride);
                    float DistA = (ParticleA->Location + WorldOffset - *ViewLocation).SizeSquared();
                    float DistB = (ParticleB->Location + WorldOffset - *ViewLocation).SizeSquared();
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
        if (Particle)
        {
            // RelativeTime 기반 수명 만료 또는 Dead 플래그 설정 시 제거
            bool bShouldKill = (Particle->RelativeTime >= 1.0f) ||
                               (Particle->Flags & EParticleFlags::Dead);
            if (bShouldKill)
            {
                KillParticle(i);
            }
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
        // Swap: LastActive의 DataIndex를 죽을 위치로
        const int32 DataIndexToFree = ParticleIndices[ActiveIndex];
        ParticleIndices[ActiveIndex] = ParticleIndices[LastActive];

        // Pop: 죽은 DataIndex를 맨 뒤로 (재사용을 위해)
        ParticleIndices[LastActive] = DataIndexToFree;
    }
    ActiveParticles = LastActive;
}

void FParticleEmitterInstance::Reset()
{
    ActiveParticles = 0;
    SpawnFraction = 0.0f;
    EmitterTime = 0.0f;
    LoopCount = 0;
    ParticleCounter = 0;
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
            EmitterDuration = 0.0f;
            EmitterTime = 0.0f;
            const uint32 BaseStride = AlignUp(sizeof(FBaseParticle), ParticleStrideAlignment);
            ReallocateParticleData(BaseStride, MaxActiveParticles);
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
    if (CurrentLODLevel && CurrentLODLevel->RequiredModule)
    {
        EmitterDuration = FMath::Max(CurrentLODLevel->RequiredModule->EmitterDuration, 0.0f);
        if (EmitterDuration > 0.0f)
        {
            EmitterTime = FMath::Clamp(EmitterTime, 0.0f, EmitterDuration);
        }
        else
        {
            EmitterTime = 0.0f;
        }
    }
    else
    {
        EmitterDuration = 0.0f;
        EmitterTime = 0.0f;
    }

    const uint32 NewStride = CalculateParticleStride();
    ReallocateParticleData(NewStride, MaxActiveParticles);
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

void FParticleEmitterInstance::ClearParticleData()
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
    ActiveParticles = 0;
}

bool FParticleEmitterInstance::CanSpawnThisFrame(float DeltaTime)
{
    if (!CurrentLODLevel || !CurrentLODLevel->RequiredModule)
    {
        return false;
    }

    EmitterTime += DeltaTime;

    const int32 LoopLimit = CurrentLODLevel->RequiredModule->EmitterLoops;
    const bool bHasFiniteLoops = (LoopLimit > 0);

    if (EmitterDuration > 0.0f)
    {
        while (EmitterTime >= EmitterDuration)
        {
            EmitterTime -= EmitterDuration;
            ++LoopCount;
            if (bHasFiniteLoops && LoopCount >= LoopLimit)
            {
                return false;
            }
        }
    }
    else if (bHasFiniteLoops && LoopCount >= LoopLimit)
    {
        return false;
    }

    return true;
}

bool FParticleEmitterInstance::UseLocalSpace() const
{
    if (CurrentLODLevel && CurrentLODLevel->RequiredModule)
    {
        return CurrentLODLevel->RequiredModule->bUseLocalSpace;
    }
    return true; // 기본값: 로컬 공간
}

FVector FParticleEmitterInstance::GetComponentWorldLocation() const
{
    if (Component)
    {
        return Component->GetWorldLocation();
    }
    return FVector::Zero();
}

void* FParticleEmitterInstance::GetModuleInstanceData(UParticleModule* Module)
{
    if (!ModuleInstanceData || !Module)
    {
        return nullptr;
    }

    // EventGenerator만 인스턴스 데이터 지원 (현재)
    if (CurrentLODLevel && CurrentLODLevel->EventGenerator == Module)
    {
        return ModuleInstanceData;
    }

    return nullptr;
}

void FParticleEmitterInstance::InitModuleInstanceData()
{
    // 기존 데이터 해제
    ClearModuleInstanceData();

    // EventGenerator가 있으면 인스턴스 데이터 할당
    if (CurrentLODLevel && CurrentLODLevel->EventGenerator)
    {
        uint32 RequiredBytes = CurrentLODLevel->EventGenerator->RequiredBytes();
        if (RequiredBytes > 0)
        {
            ModuleInstanceData = new uint8[RequiredBytes];
            ModuleInstanceDataStride = RequiredBytes;
            std::memset(ModuleInstanceData, 0, RequiredBytes);
        }
    }
}

void FParticleEmitterInstance::ClearModuleInstanceData()
{
    if (ModuleInstanceData)
    {
        delete[] ModuleInstanceData;
        ModuleInstanceData = nullptr;
    }
    ModuleInstanceDataStride = 0;
}
