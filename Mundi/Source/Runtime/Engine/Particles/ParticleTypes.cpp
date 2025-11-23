#include "pch.h"
#include "ParticleTypes.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"

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

uint32 FParticleEmitterInstance::CalculateParticleStride() const
{
    uint32 ParticleSize = sizeof(FBaseParticle);

    if (CurrentLODLevel)
    {
        ParticleSize += CurrentLODLevel->GetRequiredBytes();
    }

    return AlignUp(ParticleSize, ParticleStrideAlignment);
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

void FParticleEmitterInstance::SetLODLevel(int32 LODIndex)
{
    if (!EmitterTemplate || LODIndex < 0)
    {
        CurrentLODLevel = nullptr;
        return;
    }

    UParticleLODLevel* LOD = EmitterTemplate->GetLODLevel(LODIndex);
    CurrentLODLevel = LOD;
}
