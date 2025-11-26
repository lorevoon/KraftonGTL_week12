#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "Modules/ParticleModuleRequired.h"
#include "Modules/TypeData/ParticleModuleTypeDataBase.h"
#include "ObjectFactory.h"
#include <limits>

namespace
{
    // 모듈/타입데이터를 직렬화 기반으로 복제 (딥 카피)
    template<typename T>
    T* DuplicateViaSerialization(T* Source)
    {
        if (!Source)
        {
            return nullptr;
        }

        UClass* SourceClass = Source->GetClass();
        UObject* NewObjBase = ObjectFactory::NewObject(SourceClass);
        T* NewTypedObj = Cast<T>(NewObjBase);
        if (!NewTypedObj)
        {
            if (NewObjBase)
            {
                ObjectFactory::DeleteObject(NewObjBase);
            }
            return nullptr;
        }

        JSON TempJson = JSON::Make(JSON::Class::Object);
        Source->Serialize(false, TempJson);
        NewTypedObj->Serialize(true, TempJson);
        return NewTypedObj;
    }

    // LODLevel을 딥 카피하여 새 인덱스로 생성
    UParticleLODLevel* DuplicateLODLevel(UParticleLODLevel* SourceLOD, int32 NewLevelIndex)
    {
        if (!SourceLOD)
        {
            return nullptr;
        }

        UParticleLODLevel* NewLOD = NewObject<UParticleLODLevel>();
        NewLOD->Level = NewLevelIndex;
        NewLOD->DistanceThreshold = SourceLOD->DistanceThreshold;

        NewLOD->RequiredModule = Cast<UParticleModuleRequired>(DuplicateViaSerialization(SourceLOD->RequiredModule));
        NewLOD->TypeDataModule = Cast<UParticleModuleTypeDataBase>(DuplicateViaSerialization(SourceLOD->TypeDataModule));

        for (UParticleModule* Module : SourceLOD->Modules)
        {
            NewLOD->Modules.Add(Cast<UParticleModule>(DuplicateViaSerialization(Module)));
        }

        return NewLOD;
    }

    // Emitters가 LODDistances 개수에 맞게 LODLevel 수를 맞추도록 보정
    void EnsureEmitterLODCount(UParticleEmitter* Emitter, int32 TargetCount, const TArray<float>& LODDistances)
    {
        if (!Emitter)
        {
            return;
        }

        // 부족한 LODLevel은 직전 레벨을 복제하여 채움
        while (Emitter->LODLevels.Num() < TargetCount)
        {
            const int32 NewIndex = Emitter->LODLevels.Num();
            UParticleLODLevel* SourceLOD = Emitter->LODLevels.IsEmpty() ? nullptr : Emitter->LODLevels.Last();
            UParticleLODLevel* NewLOD = DuplicateLODLevel(SourceLOD, NewIndex);
            if (!NewLOD)
            {
                NewLOD = NewObject<UParticleLODLevel>();
                if (NewLOD)
                {
                    NewLOD->Level = NewIndex;
                }
            }
            if (NewLOD)
            {
                NewLOD->DistanceThreshold = (NewIndex < LODDistances.Num()) ? LODDistances[NewIndex] : 0.0f;
            }
            Emitter->LODLevels.Add(NewLOD);
        }

        // 남는 LODLevel은 제거
        while (Emitter->LODLevels.Num() > TargetCount)
        {
            const int32 RemoveIndex = Emitter->LODLevels.Num() - 1;
            UParticleLODLevel* Removed = Emitter->LODLevels[RemoveIndex];
            if (Removed)
            {
                ObjectFactory::DeleteObject(Removed);
            }
            Emitter->LODLevels.RemoveAt(RemoveIndex);
        }

        // Level 인덱스 정규화
        for (int32 Index = 0; Index < Emitter->LODLevels.Num(); ++Index)
        {
            if (Emitter->LODLevels[Index])
            {
                Emitter->LODLevels[Index]->Level = Index;
                if (Index < LODDistances.Num())
                {
                    Emitter->LODLevels[Index]->DistanceThreshold = LODDistances[Index];
                }
            }
        }
    }
}

UParticleSystem::UParticleSystem()
    : SystemName("ParticleSystem")
    , SystemDuration(0.0f)
    , SystemLoops(0)
    , bAutoActivate(true)
    , LODDistanceCheckTime(0.25f)
    , LODMethod(EParticleSystemLODMethod::Automatic)
{
    // LOD 0은 항상 0.0f
    LODDistances.Add(0.0f);
}

void UParticleSystem::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    JSON ParticleJson;
    std::wstring WidePath(InFilePath.begin(), InFilePath.end());
    if (FJsonSerializer::LoadJsonFromFile(ParticleJson, WidePath))
    {
        Serialize(true, ParticleJson);
        try
        {
            std::filesystem::path FsPath(WidePath);
            SetLastModifiedTime(std::filesystem::last_write_time(FsPath));
        }
        catch (...)
        {
        }
    }
}

UParticleEmitter* UParticleSystem::GetEmitter(int32 Index) const
{
    if (Index >= 0 && Index < Emitters.Num())
    {
        return Emitters[Index];
    }
    return nullptr;
}

void UParticleSystem::AddEmitter(UParticleEmitter* Emitter)
{
    if (Emitter)
    {
        Emitters.Add(Emitter);
        // 새 이미터도 현재 LOD 개수에 맞춰 LODLevels를 정렬
        EnsureEmitterLODCount(Emitter, LODDistances.Num(), LODDistances);
        bIsDirty = true;
    }
}

void UParticleSystem::RemoveEmitter(UParticleEmitter* Emitter)
{
    if (Emitter)
    {
        Emitters.Remove(Emitter);
        bIsDirty = true;
    }
}

void UParticleSystem::SwapEmitters(int32 IndexA, int32 IndexB)
{
    if (IndexA >= 0 && IndexA < Emitters.Num() && IndexB >= 0 && IndexB < Emitters.Num())
    {
        UParticleEmitter* Temp = Emitters[IndexA];
        Emitters[IndexA] = Emitters[IndexB];
        Emitters[IndexB] = Temp;
        bIsDirty = true;
    }
}

void UParticleSystem::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // UPROPERTY 필드는 자동 직렬화되므로 수동 처리 대상만 별도 처리

        // Load LOD distances
        LODDistances.Empty();
        if (InOutHandle.hasKey("LODDistances"))
        {
            const JSON& LODArray = InOutHandle.at("LODDistances");
            if (LODArray.JSONType() == JSON::Class::Array)
            {
                for (const JSON& Elem : LODArray.ArrayRange())
                {
                    if (Elem.JSONType() == JSON::Class::Floating || Elem.JSONType() == JSON::Class::Integral)
                    {
                        LODDistances.Add(static_cast<float>(Elem.ToFloat()));
                    }
                }
            }
        }
        if (LODDistances.IsEmpty())
        {
            LODDistances.Add(0.0f);
        }
        else
        {
            LODDistances[0] = 0.0f; // LOD0은 항상 0
        }

        // Load emitters array
        if (InOutHandle.hasKey("Emitters"))
        {
            const JSON& EmittersArray = InOutHandle.at("Emitters");
            if (EmittersArray.JSONType() == JSON::Class::Array)
            {
                Emitters.Empty();
                for (const JSON& EmitterJson : EmittersArray.ArrayRange())
                {
                    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
                    if (Emitter)
                    {
                        JSON EmitterJsonCopy = EmitterJson;
                        Emitter->Serialize(bInIsLoading, EmitterJsonCopy);
                        Emitters.Add(Emitter);
                    }
                }
            }
        }

        // LOD 개수에 맞춰 각 이미터 LODLevels 보정
        const int32 TargetLODCount = LODDistances.Num();
        for (UParticleEmitter* Emitter : Emitters)
        {
            EnsureEmitterLODCount(Emitter, TargetLODCount, LODDistances);
        }
    }
    else
    {
        // UPROPERTY 필드는 자동 직렬화됨. 수동 직렬화 대상만 저장.

        // Save LOD distances
        JSON LODArray = JSON::Make(JSON::Class::Array);
        for (float Dist : LODDistances)
        {
            LODArray.append(static_cast<double>(Dist));
        }
        InOutHandle["LODDistances"] = LODArray;

        // Save emitters array
        JSON EmittersArray = JSON::Make(JSON::Class::Array);
        for (UParticleEmitter* Emitter : Emitters)
        {
            if (Emitter)
            {
                JSON EmitterJson = JSON::Make(JSON::Class::Object);
                Emitter->Serialize(bInIsLoading, EmitterJson);
                EmittersArray.append(EmitterJson);
            }
        }
        InOutHandle["Emitters"] = EmittersArray;
    }
}

// LOD 거리 설정. 1 ~ N 인덱스만 설정 가능 (LOD 0은 무조건 0.0f. N 입력시 새로 추가)
    // 설정 성공시 true 반환, 실패시 false 반환
bool UParticleSystem::SetLODDistance(int32 LODIndex, float Distance)
{
    if (LODIndex <= 0)
    {
        return false; // LOD 0은 항상 0.0f
    }

    // 정렬 제약: 이전/다음 값 사이에 있어야 함
    const int32 CurrentCount = LODDistances.Num();
    const float PrevDistance = (LODIndex - 1 < CurrentCount) ? LODDistances[LODIndex - 1] : 0.0f;
    const float NextDistance = (LODIndex + 1 < CurrentCount) ? LODDistances[LODIndex + 1] : std::numeric_limits<float>::infinity();
    if (Distance < PrevDistance || Distance > NextDistance)
    {
        return false; // 오름차순 제약 위반
    }

    if (LODIndex < LODDistances.Num())
    {
        LODDistances[LODIndex] = Distance;
        return true;
    }

    if (LODIndex == LODDistances.Num())
    {
        // 신규 LOD 추가
        LODDistances.Add(Distance);

        const int32 TargetLODCount = LODDistances.Num();
        for (UParticleEmitter* Emitter : Emitters)
        {
            EnsureEmitterLODCount(Emitter, TargetLODCount, LODDistances);
        }

        return true;
    }

    return false; // 건너뛴 인덱스는 허용하지 않음
}

bool UParticleSystem::AddLODDistance(float Distance)
{
    // 새 LOD는 Set 경로를 거쳐 정렬 검증을 통과하도록 함
    return SetLODDistance(LODDistances.Num(), Distance);
}

bool UParticleSystem::InsertLODDistance(int32 LODIndex, float Distance)
{
    // 유효 범위: 1 ~ Num (LODIndex == Num이면 push-back과 동일)
    if (LODIndex <= 0 || LODIndex > LODDistances.Num())
    {
        return false;
    }

    // 정렬 제약 확인: 이전/다음 거리 사이인지 검사
    const float PrevDistance = (LODIndex - 1 < LODDistances.Num()) ? LODDistances[LODIndex - 1] : 0.0f;
    const float NextDistance = (LODIndex < LODDistances.Num()) ? LODDistances[LODIndex] : std::numeric_limits<float>::infinity();
    if (Distance < PrevDistance || Distance > NextDistance)
    {
        return false;
    }

    // 삽입
    LODDistances.Insert(Distance, LODIndex);

    // 모든 이미터의 LODLevels 갱신
    const int32 TargetLODCount = LODDistances.Num();
    for (UParticleEmitter* Emitter : Emitters)
    {
        EnsureEmitterLODCount(Emitter, TargetLODCount, LODDistances);
    }

    return true;
}

bool UParticleSystem::RemoveLODDistance(int32 LODIndex)
{
    // LOD 0 제거는 허용하지 않음
    if (LODIndex <= 0 || LODIndex >= LODDistances.Num())
    {
        return false;
    }

    LODDistances.RemoveAt(LODIndex);

    const int32 TargetLODCount = LODDistances.Num();
    for (UParticleEmitter* Emitter : Emitters)
    {
        if (!Emitter)
        {
            continue;
        }

        if (Emitter->LODLevels.Num() > LODIndex)
        {
            UParticleLODLevel* RemovedLOD = Emitter->LODLevels[LODIndex];
            if (RemovedLOD)
            {
                ObjectFactory::DeleteObject(RemovedLOD);
            }
            Emitter->LODLevels.RemoveAt(LODIndex);
        }

        EnsureEmitterLODCount(Emitter, TargetLODCount, LODDistances);
    }

    return true;
}
