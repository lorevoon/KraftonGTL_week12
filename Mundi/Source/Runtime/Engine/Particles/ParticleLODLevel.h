#pragma once

#include "Object.h"
#include "ParticleModule.h"
#include "Modules/ParticleModuleRequired.h"
#include "UParticleLODLevel.generated.h"

// 전방 선언
class UParticleModuleTypeDataBase;

// LOD 레벨별 모듈 리스트
// - LOD별로 다른 모듈 리스트 보유
// - 실행 단계별로 모듈을 필터링하여 반환
UCLASS(DisplayName="파티클 LOD 레벨", Description="LOD 레벨별 파티클 모듈 리스트입니다")
class UParticleLODLevel : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleLODLevel();

    // 필수 모듈 (모든 이미터가 반드시 가져야 함)
    UPROPERTY(EditAnywhere, Category="Required")
    UParticleModuleRequired* RequiredModule;

    // 타입 데이터 모듈 (이미터 타입 결정)
    // - nullptr이면 Sprite (기본)
    // - ParticleModuleTypeDataMesh면 Mesh
    UPROPERTY(EditAnywhere, Category="TypeData")
    UParticleModuleTypeDataBase* TypeDataModule;

    // 옵션 모듈 리스트
    UPROPERTY(EditAnywhere, Category="Modules")
    TArray<UParticleModule*> Modules;

    // LOD 레벨 인덱스
    UPROPERTY(EditAnywhere, Category="LOD")
    int32 Level;

    // LOD 적용 거리 (레벨이 높을수록 먼 거리)
    UPROPERTY(EditAnywhere, Category="LOD")
    float DistanceThreshold;

public:
    // 실행 단계별 모듈 리스트 반환
    // - 캐싱 없이 매번 필터링 (간단한 구현)
    // - 추후 최적화 가능: 모듈 변경 시 캐시 갱신

    // Spawn 단계에서 실행할 모듈 리스트 반환
    TArray<UParticleModule*> GetSpawnModules() const;

    // Update 단계에서 실행할 모듈 리스트 반환
    TArray<UParticleModule*> GetUpdateModules() const;

    // FinalUpdate 단계에서 실행할 모듈 리스트 반환
    TArray<UParticleModule*> GetFinalUpdateModules() const;

    // 모든 모듈 반환 (RequiredModule + Modules)
    TArray<UParticleModule*> GetAllModules() const;

    // 모듈 추가/제거
    void AddModule(UParticleModule* Module);
    void RemoveModule(UParticleModule* Module);
};
