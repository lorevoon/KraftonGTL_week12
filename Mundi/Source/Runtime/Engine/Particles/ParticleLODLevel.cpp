#include "pch.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"

UParticleLODLevel::UParticleLODLevel()
    : RequiredModule(nullptr)
    , TypeDataModule(nullptr)
    , Level(0)
    , DistanceThreshold(0.0f)
{
}

// 실행 단계별 모듈 필터링
// - bSpawnModule == true인 모듈만 반환
TArray<UParticleModule*> UParticleLODLevel::GetSpawnModules() const
{
    TArray<UParticleModule*> SpawnModules;

    // RequiredModule은 항상 첫 번째로 실행
    if (RequiredModule && RequiredModule->bEnabled && RequiredModule->bSpawnModule)
    {
        SpawnModules.Add(RequiredModule);
    }

    // 나머지 모듈 중 bSpawnModule == true인 것만 추가
    for (UParticleModule* Module : Modules)
    {
        if (Module && Module->bEnabled && Module->bSpawnModule)
        {
            SpawnModules.Add(Module);
        }
    }

    return SpawnModules;
}

// 실행 단계별 모듈 필터링
// - bUpdateModule == true인 모듈만 반환
TArray<UParticleModule*> UParticleLODLevel::GetUpdateModules() const
{
    TArray<UParticleModule*> UpdateModules;

    // RequiredModule도 Update 단계에 참여할 수 있음
    if (RequiredModule && RequiredModule->bEnabled && RequiredModule->bUpdateModule)
    {
        UpdateModules.Add(RequiredModule);
    }

    for (UParticleModule* Module : Modules)
    {
        if (Module && Module->bEnabled && Module->bUpdateModule)
        {
            UpdateModules.Add(Module);
        }
    }

    return UpdateModules;
}

// 실행 단계별 모듈 필터링
// - bFinalUpdateModule == true인 모듈만 반환
TArray<UParticleModule*> UParticleLODLevel::GetFinalUpdateModules() const
{
    TArray<UParticleModule*> FinalUpdateModules;

    if (RequiredModule && RequiredModule->bEnabled && RequiredModule->bFinalUpdateModule)
    {
        FinalUpdateModules.Add(RequiredModule);
    }

    for (UParticleModule* Module : Modules)
    {
        if (Module && Module->bEnabled && Module->bFinalUpdateModule)
        {
            FinalUpdateModules.Add(Module);
        }
    }

    return FinalUpdateModules;
}

// 모든 모듈 반환 (디버깅/에디터 용)
TArray<UParticleModule*> UParticleLODLevel::GetAllModules() const
{
    TArray<UParticleModule*> AllModules;

    if (RequiredModule)
    {
        AllModules.Add(RequiredModule);
    }

    for (UParticleModule* Module : Modules)
    {
        if (Module)
        {
            AllModules.Add(Module);
        }
    }

    return AllModules;
}

uint32 UParticleLODLevel::GetRequiredBytes() const
{
    uint32 TotalBytes = 0;

    if (RequiredModule && RequiredModule->bEnabled)
    {
        TotalBytes += RequiredModule->RequiredBytes();
    }

    for (UParticleModule* Module : Modules)
    {
        if (Module && Module->bEnabled)
        {
            TotalBytes += Module->RequiredBytes();
        }
    }

    return TotalBytes;
}

void UParticleLODLevel::AddModule(UParticleModule* Module)
{
    if (Module)
    {
        Modules.Add(Module);
    }
}

void UParticleLODLevel::RemoveModule(UParticleModule* Module)
{
    if (Module)
    {
        Modules.Remove(Module);
    }
}
