#pragma once

#include "ResourceBase.h"
#include "ParticleEmitter.h"
#include "UParticleSystem.generated.h"

/** @brief: 파티클의 LOD 계산 방식을 정의합니다.*/
enum class EParticleSystemLODMethod : uint8
{
    Automatic = 0,        // 일정 주기마다(LODDistanceCheckTime) 거리 기반으로 LOD를 자동으로 갱신
    DirectSet = 1,        // 게임 코드에서 LODIndex를 직접 세팅
    ActivateAutomatic = 2 // 처음 활성화될 때 한 번 거리 기반으로 LOD를 정하고, 그 이후에는 코드로 직접 바꾸지 않는 한 고정
};

// 파티클 시스템 에셋 (최상위)
// - 여러 이미터를 포함하는 컨테이너
// - 에디터에서 편집 가능, 런타임에 여러 인스턴스 공유
UCLASS(DisplayName="파티클 시스템", Description="파티클 이미터들을 포함하는 에셋입니다")
class UParticleSystem : public UResourceBase
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleSystem();

    // Load from disk via ResourceManager
    void Load(const FString& InFilePath, ID3D11Device* InDevice);

    // 이미터 배열
    // - 각 이미터는 독립적인 파티클 그룹 (예: 불꽃, 연기, 불똥)
    UPROPERTY(EditAnywhere, Category="Emitters")
    TArray<UParticleEmitter*> Emitters;

    // 시스템 이름
    UPROPERTY(EditAnywhere, Category="System")
    FString SystemName;

    // 전체 시스템 지속 시간 (0 = 무한)
    UPROPERTY(EditAnywhere, Category="System")
    float SystemDuration;

    // 시스템 반복 횟수 (0 = 무한)
    UPROPERTY(EditAnywhere, Category="System")
    int32 SystemLoops;

    // 자동 재생 여부
    UPROPERTY(EditAnywhere, Category="System")
    bool bAutoActivate;

	// LOD 거리 체크 주기 기본값 (초) (컴포넌트에서 오버라이드 가능)
    UPROPERTY(EditAnywhere, Category = "system")
    float LODDistanceCheckTime;

	// Editor/runtime change tracking for live preview rebuilds
	UPROPERTY()
	bool bIsDirty = false;

	// LOD 메소드 기본값 (컴포넌트에서 오버라이드 가능)
	UPROPERTY(EditAnywhere, Category = "System")
    EParticleSystemLODMethod LODMethod;

	// LOD 거리 배열 (각 LOD 레벨로 전환되는 최소 거리)
    // LODDistances[0] (LOD 0)은 항상 0
    TArray<float> LODDistances;

    // 이미터 접근
    UParticleEmitter* GetEmitter(int32 Index) const;
    void AddEmitter(UParticleEmitter* Emitter);
    void RemoveEmitter(UParticleEmitter* Emitter);
    void SwapEmitters(int32 IndexA, int32 IndexB);
    int32 GetEmitterCount() const { return Emitters.Num(); }

    // LOD 메소드
	int32 GetLODLevelCount() const { return LODDistances.Num(); }

	// LOD 거리 설정. 1 ~ N 인덱스만 설정 가능 (LOD 0은 무조건 0.0f. N 입력시 새로 추가)
	// 설정 성공시 true 반환, 실패시 false 반환
    bool SetLODDistance(int32 LODIndex, float Distance);
    bool AddLODDistance(float Distance);
    bool RemoveLODDistance(int32 LODIndex);
    bool InsertLODDistance(int32 LODIndex, float Distance);

    EParticleSystemLODMethod GetLODMethod() const { return LODMethod; }
    void SetLODMethod(EParticleSystemLODMethod InMethod) { LODMethod = InMethod; }

	// Mark dirty to signal preview/components to rebuild
	inline void MarkDirty() { bIsDirty = true; }
	
    // Serialization
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
