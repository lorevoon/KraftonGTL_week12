#include "pch.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "Source/Runtime/Core/Object/Object.h"
#include "Source/Runtime/Core/Object/ObjectFactory.h"
#include "Modules/ParticleModuleRequired.h"
#include "Modules/TypeData/ParticleModuleTypeDataBase.h"

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

void UParticleLODLevel::SwapModules(int32 IndexA, int32 IndexB)
{
    if (IndexA >= 0 && IndexA < Modules.Num() && IndexB >= 0 && IndexB < Modules.Num())
    {
        UParticleModule* Temp = Modules[IndexA];
        Modules[IndexA] = Modules[IndexB];
        Modules[IndexB] = Temp;
    }
}

// Helper function to create module from type name
static UParticleModule* CreateModuleFromTypeName(const FString& TypeName)
{
    // Prefer reflection-based construction over hardcoded conditionals
    if (TypeName.empty()) return nullptr;
    UClass* ModuleClass = UClass::FindClass(FName(TypeName));
    if (!ModuleClass || !ModuleClass->IsChildOf(UParticleModule::StaticClass()))
    {
        return nullptr;
    }
    UObject* Obj = ObjectFactory::NewObject(ModuleClass);
    return Cast<UParticleModule>(Obj);
}

void UParticleLODLevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Load
        int32 LevelValue;
        float DistValue;
        FJsonSerializer::ReadInt32(InOutHandle, "Level", LevelValue, 0, false);
        FJsonSerializer::ReadFloat(InOutHandle, "DistanceThreshold", DistValue, 0.0f, false);
        Level = LevelValue;
        DistanceThreshold = DistValue;

        // Load required module
        if (InOutHandle.hasKey("RequiredModule"))
        {
            const JSON& ReqModuleJson = InOutHandle.at("RequiredModule");
            if (ReqModuleJson.JSONType() == JSON::Class::Object && ReqModuleJson.hasKey("Type"))
            {
                FString TypeName;
                FJsonSerializer::ReadString(ReqModuleJson, "Type", TypeName, "", false);
                RequiredModule = static_cast<UParticleModuleRequired*>(CreateModuleFromTypeName(TypeName));
                if (RequiredModule)
                {
                    JSON ReqModuleCopy = ReqModuleJson;
                    RequiredModule->Serialize(bInIsLoading, ReqModuleCopy);
                }
            }
        }

        // Load TypeData module (optional)
        if (InOutHandle.hasKey("TypeDataModule"))
        {
            const JSON& TypeDataJson = InOutHandle.at("TypeDataModule");
            if (TypeDataJson.JSONType() == JSON::Class::Object && TypeDataJson.hasKey("Type"))
            {
                FString TypeName;
                FJsonSerializer::ReadString(TypeDataJson, "Type", TypeName, "", false);
                UParticleModule* Created = CreateModuleFromTypeName(TypeName);
                // Only accept TypeData subclasses
                if (Created && Created->GetClass()->IsChildOf(UParticleModuleTypeDataBase::StaticClass()))
                {
                    TypeDataModule = static_cast<UParticleModuleTypeDataBase*>(Created);
                    JSON TypeDataCopy = TypeDataJson;
                    TypeDataModule->Serialize(bInIsLoading, TypeDataCopy);
                }
                else if (Created)
                {
                    // Not a valid TypeData, discard the created module
                    ObjectFactory::DeleteObject(Created);
                }
            }
        }

        // Load modules array
        if (InOutHandle.hasKey("Modules"))
        {
            const JSON& ModulesArray = InOutHandle.at("Modules");
            if (ModulesArray.JSONType() == JSON::Class::Array)
            {
                Modules.Empty();
                for (const JSON& ModuleJson : ModulesArray.ArrayRange())
                {
                    if (ModuleJson.hasKey("Type"))
                    {
                        FString TypeName;
                        FJsonSerializer::ReadString(ModuleJson, "Type", TypeName, "", false);
                        UParticleModule* Module = CreateModuleFromTypeName(TypeName);
                        if (Module)
                        {
                            JSON ModuleJsonCopy = ModuleJson;
                            Module->Serialize(bInIsLoading, ModuleJsonCopy);
                            Modules.Add(Module);
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Save
        InOutHandle["Level"] = static_cast<long>(Level);
        InOutHandle["DistanceThreshold"] = static_cast<double>(DistanceThreshold);

        // Save required module
        if (RequiredModule)
        {
            JSON ReqModuleJson = JSON::Make(JSON::Class::Object);
            RequiredModule->Serialize(bInIsLoading, ReqModuleJson);
            InOutHandle["RequiredModule"] = ReqModuleJson;
        }

        // Save TypeData module (optional)
        if (TypeDataModule)
        {
            JSON TypeDataJson = JSON::Make(JSON::Class::Object);
            TypeDataModule->Serialize(bInIsLoading, TypeDataJson);
            InOutHandle["TypeDataModule"] = TypeDataJson;
        }

        // Save modules array
        JSON ModulesArray = JSON::Make(JSON::Class::Array);
        for (UParticleModule* Module : Modules)
        {
            if (Module)
            {
                JSON ModuleJson = JSON::Make(JSON::Class::Object);
                Module->Serialize(bInIsLoading, ModuleJson);
                ModulesArray.append(ModuleJson);
            }
        }
        InOutHandle["Modules"] = ModulesArray;
    }
}
