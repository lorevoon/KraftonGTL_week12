#pragma once

#include <cstdint>
#include "Vector.h"
#include "Color.h"
#include "ParticleTypes.h"

// 전방 선언
class UParticleEmitter;
class UParticleLODLevel;
class UParticleSystemComponent;
class FSceneView;
class UParticleModule;

// 파티클 이미터 인스턴스 (런타임)
// - 실제 파티클 시뮬레이션을 담당하는 비-UObject 구조체
// - 메모리 풀 관리 및 파티클 생명주기 제어
struct FParticleEmitterInstance
{
    static constexpr uint32 ParticleStrideAlignment = 16u; // 파티클 메모리 정렬 단위

    UParticleEmitter* EmitterTemplate;       // 에셋 참조
    UParticleSystemComponent* Component;     // 소유 컴포넌트
    UParticleLODLevel* CurrentLODLevel;      // 현재 LOD
	int32 CurrentLODLevelIndex;              // 현재 LOD 인덱스 (0 = highest quality)

    // 메모리 관리
    uint8* ParticleData;                           // 연속된 파티클 메모리 블록
    int32* ParticleIndices;                        // 활성 파티클 인덱스 배열 (논리 인덱스 -> 데이터 배열 실제 인덱스 매핑)
    uint32 ParticleStride;                         // sizeof(FBaseParticle) + 추가 페이로드
    int32 ActiveParticles;                         // 현재 활성 파티클 수
    int32 MaxActiveParticles;                      // 최대 파티클 수

    // 스폰 제어
    float SpawnFraction;                           // 누적 스폰 잔량
    float SecondsSinceCreation;                    // 생성 후 경과 시간

    FParticleEmitterInstance();
    ~FParticleEmitterInstance();

    /**
     * @brief 주어진 값과 정렬 단위에 맞춰 값을 올림 정렬합니다.
     * @note Alignment는 2의 거듭제곱이어야 합니다.
     */
    static uint32 AlignUp(uint32 Value, uint32 Alignment)
    {
        const uint32 Mask = Alignment - 1u;
        return (Value + Mask) & ~Mask;
    }

    // 메모리 할당
    void InitParticles(int32 InMaxParticles);

    /**
	 * @brief 상위 초기화를 수행합니다. (템플릿/컴포넌트/LOD와 메모리 풀 준비)
	 * @param InTemplate 이미터 템플릿
	 * @param InComponent 소유 파티클 시스템 컴포넌트
	 * @param InLODIndex 초기 LOD 인덱스
	 * @param InMaxActiveParticles 최대 활성 파티클 수 (0이하 값을 주면 템플릿 설정을 사용합니다)
     */
    void Initialize(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, int32 InLODIndex, int32 InMaxActiveParticles);

    /** @brief: 모듈 요구 바이트를 합산해 정렬(align)까지 고려한 Stride를 계산합니다. */
    uint32 CalculateParticleStride() const;

    /** @brief: LOD 변경 등으로 Stride가 달라질 때 기존 파티클 메모리를 정리하고 새 Stride로 재할당 */
    void ReallocateParticleData(uint32 NewStride);

    /** @brief 한 프레임 틱 업데이트를 수행합니다. (스폰 → 업데이트 → 파이널 업데이트 → Kill) */
    void Tick(float DeltaTime);

    // SpawnRate 기반 파티클 생성
    void SpawnParticles(float DeltaTime);

    // Update 단계 모듈 실행
    void RunUpdateModules(float DeltaTime);

    // FinalUpdate 단계 모듈 실행
    void RunFinalUpdateModules(float DeltaTime);

    // 파티클 정렬
    void Sort(EParticleSortMode SortMode = EParticleSortMode::None, const FVector* ViewLocation = nullptr);

    // 사망 파티클 정리
    void KillDeadParticles();

    // 특정 활성 인덱스 파티클 제거
    void KillParticle(int32 ActiveIndex);

    // 상태 리셋(메모리 유지)
    void Reset();

    // LOD 전환
    void SetLODLevel(int32 LODIndex);

    // Stride 기반 파티클 접근
    FBaseParticle* GetParticle(int32 ActiveIndex);
    const FBaseParticle* GetParticle(int32 ActiveIndex) const;
};
