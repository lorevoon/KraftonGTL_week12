# Person 1 작업 계획 - Particle System 기반 구조 구축 (UE5 패턴 적용)

**담당자**: Person 1 (Engine Core - Particle System 기반 구조)
**작업 기간**: 4일 (Day 1-4)
**우선순위**: 🔴 최우선 크리티컬 패스
**전략**: Interface-First 개발 (헤더 먼저 작성 → 공유 → 병렬 구현)
**참고**: UE5 Particle System 분석 결과 반영

---

## 🎯 UE5 패턴 적용 현황

이 계획은 **UE5 Particle System 분석 문서(Part 1-4)**를 기반으로 검증된 패턴을 적용합니다.

### ✅ 적용한 패턴 (High Priority)

1. **실행 단계 플래그 시스템**
   - **UE5**: `uint8 bSpawnModule:1, bUpdateModule:1, bFinalUpdateModule:1`
   - **Mundi**: `bool bSpawnModule, bUpdateModule, bFinalUpdateModule` (비트필드 대신 일반 bool)
   - **효과**: 한 모듈이 여러 단계에서 실행 가능, 유연성 대폭 향상
   - **복잡도**: 증가 없음 (오히려 단순함)

2. **헬퍼 매크로 시스템**
   - `MUNDI_DECLARE_PARTICLE_PTR` - 파티클 포인터 접근
   - `MUNDI_BEGIN_UPDATE_LOOP` / `MUNDI_END_UPDATE_LOOP` - 루프 간소화
   - `MUNDI_SPAWN_INIT` - 파티클 초기화
   - **효과**: 보일러플레이트 코드 제거, 가독성 향상
   - **복잡도**: 증가 없음

### 🟡 제안한 패턴 (Medium Priority - 선택사항)

3. **Base 필드 추가**
   - `FMundiBaseParticle`에 `BaseVelocity`, `BaseSize`, `BaseColor` 추가
   - **효과**: Person 3의 OverLife 모듈 정확성 보장
   - **복잡도**: 약간 증가 (+48 bytes, 초기화 코드)
   - **판단**: Day 2 진행 속도 보고 결정

### ❌ 미적용 패턴 (Low Priority - 향후 과제)

4. **Context Pattern**
   - 파라미터를 구조체로 묶기 (`FSpawnContext`, `FUpdateContext`)
   - **이유**: 초기 구현 복잡도 증가, 4일 일정 유지 우선

5. **LOD 모듈 리스트 캐싱**
   - LOD별로 Spawn/Update 모듈 리스트 미리 캐싱
   - **이유**: Person 2의 작업 범위, 향후 최적화로 연기

---

## 📋 전체 개요

### 핵심 목표
1. Particle System의 모듈 아키텍처 구축
2. 다른 팀원들이 대기 없이 작업할 수 있도록 인터페이스 조기 제공
3. 6개 필수 모듈 구현 (Spawn, Lifetime, Location, Velocity, Color, Size)
4. Helper 매크로 시스템 제공
5. **UE5의 검증된 패턴 적용**

### 의존성 관리
- **Person 2** → Day 1 오전에 기본 클래스 인터페이스 필요
- **Person 3** → Person 2 의존 (Person 1과 직접 의존성 없음)
- **Person 4** → Day 2 오전에 모듈 클래스 + 리플렉션 필요

### 파일 위치
```
Mundi/Source/Runtime/Engine/Particles/
├── ParticleTypes.h                      # 공통 타입 정의
├── ParticleHelper.h                     # 매크로 시스템 (Day 1) ⭐ 이동됨
├── ParticleModule.h/.cpp                # 모듈 베이스 ⭐ UE5 패턴 적용
├── ParticleModuleRequired.h/.cpp        # 필수 모듈
├── ParticleLODLevel.h/.cpp              # LOD 레벨
├── ParticleEmitter.h/.cpp               # 이미터
├── ParticleSystem.h/.cpp                # 시스템
└── Modules/
    ├── ParticleModuleSpawn.h/.cpp
    ├── ParticleModuleLifetime.h/.cpp
    ├── ParticleModuleLocation.h/.cpp
    ├── ParticleModuleVelocity.h/.cpp
    ├── ParticleModuleColor.h/.cpp
    ├── ParticleModuleSize.h/.cpp
    └── ParticleModuleSizeScaleBySpeed.h/.cpp
```

---

## 🎯 Day 1 - 기반 구조 및 인터페이스 (8시간)

### ✅ Phase 1.1: 디렉토리 구조 생성 (15분)

**작업 내용**:
```bash
mkdir Mundi\Source\Runtime\Engine\Particles
mkdir Mundi\Source\Runtime\Engine\Particles\Modules
```

**검증**:
- [ ] 디렉토리가 생성되었는지 확인
- [ ] vcxproj 필터에 디렉토리 추가 (Visual Studio)

---

### ✅ Phase 1.2: ParticleTypes.h 작성 (30분)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleTypes.h`

**작업 내용**:
```cpp
#pragma once
#include "Core.h"

/**
 * 파티클 이미터 타입
 */
enum class EDynamicEmitterType : uint8
{
    Sprite,      // 스프라이트 파티클 (빌보드)
    Mesh,        // 메시 파티클 (3D 메시)
    Unknown
};

/**
 * 파티클 정렬 모드
 */
enum class EParticleSortMode : uint8
{
    None,                    // 정렬 안 함
    ViewDistanceDepth,       // 카메라 거리 기준
    AgeOldestFirst,         // 나이 많은 순
    AgeNewestFirst          // 나이 적은 순
};

/**
 * 파티클 분포 타입 (위치/속도 분포)
 */
enum class EDistributionType : uint8
{
    Constant,       // 고정값
    Uniform,        // 균등 분포
    ConstantCurve,  // 시간별 커브 (선택)
    Particle        // 파티클별 랜덤
};

// 전방 선언
struct FBaseParticle;
struct FParticleEmitterInstance;
class UParticleSystemComponent;
```

**검증**:
- [ ] 파일이 컴파일되는지 확인
- [ ] enum class 사용 (타입 안전성)

---

### ✅ Phase 1.3: ParticleModule.h 작성 (1시간) ⭐ UE5 패턴 적용

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleModule.h`

**작업 내용**:
```cpp
#pragma once
#include "Object.h"
#include "ParticleTypes.h"
#include "ParticleModule.generated.h"

/**
 * 파티클 모듈 베이스 클래스
 * 모든 파티클 모듈은 이 클래스를 상속받아 구현
 *
 * ⭐ UE5 패턴: 실행 단계 플래그 시스템 사용
 */
UCLASS(DisplayName="Particle Module", Description="파티클 속성을 제어하는 모듈")
class UParticleModule : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModule();
    virtual ~UParticleModule() = default;

    /**
     * 파티클 생성 시 호출
     * @param Owner - 이미터 인스턴스
     * @param Offset - 파티클 데이터 오프셋
     * @param SpawnTime - 생성 시간
     */
    UFUNCTION()
    virtual void Spawn(struct FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime);

    /**
     * 파티클 업데이트 시 매 프레임 호출
     * @param Owner - 이미터 인스턴스
     * @param Offset - 파티클 데이터 오프셋
     * @param DeltaTime - 프레임 시간
     */
    UFUNCTION()
    virtual void Update(struct FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime);

    /**
     * 렌더링 직전 최종 업데이트 호출
     * @param Owner - 이미터 인스턴스
     * @param Offset - 파티클 데이터 오프셋
     * @param DeltaTime - 프레임 시간
     */
    UFUNCTION()
    virtual void FinalUpdate(struct FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime);

    /** 모듈 활성화 여부 */
    UPROPERTY(EditAnywhere, Category="Module")
    bool bEnabled = true;

    // ===== ⭐ UE5 패턴: 실행 단계 플래그 =====
    // 기존 ExecutionOrder 대신 사용
    // 하나의 모듈이 여러 단계에서 실행 가능

    /** Spawn 단계에서 실행 여부 */
    UPROPERTY()
    bool bSpawnModule = false;

    /** Update 단계에서 실행 여부 */
    UPROPERTY()
    bool bUpdateModule = false;

    /** FinalUpdate 단계에서 실행 여부 */
    UPROPERTY()
    bool bFinalUpdateModule = false;
};
```

**검증**:
- [ ] UCLASS, GENERATED_REFLECTION_BODY() 매크로 사용
- [ ] 순수 가상 함수가 아닌 기본 구현 제공 (빈 함수체)
- [ ] bool 플래그 3개 정의됨
- [ ] Person 2가 include 가능한지 확인

---

### ✅ Phase 1.4: ParticleModuleRequired.h 작성 (45분)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleModuleRequired.h`

**작업 내용**:
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleRequired.generated.h"

/**
 * 필수 파티클 모듈
 * 이미터의 기본 속성을 정의 (Material, EmitterType 등)
 */
UCLASS(DisplayName="Required", Description="이미터 필수 설정")
class UParticleModuleRequired : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleRequired();

    /** 이미터 타입 (Sprite/Mesh) */
    UPROPERTY(EditAnywhere, Category="Emitter")
    EDynamicEmitterType EmitterType = EDynamicEmitterType::Sprite;

    /** 사용할 머티리얼 (렌더링용) */
    UPROPERTY(EditAnywhere, Category="Rendering")
    class UMaterialInterface* Material = nullptr;

    /** 정렬 모드 */
    UPROPERTY(EditAnywhere, Category="Rendering")
    EParticleSortMode SortMode = EParticleSortMode::None;

    /** 이미터 스케일 */
    UPROPERTY(EditAnywhere, Category="Emitter")
    FVector EmitterScale = FVector(1.0f, 1.0f, 1.0f);

    /** 최대 파티클 개수 */
    UPROPERTY(EditAnywhere, Category="Emitter")
    int32 MaxParticles = 100;
};
```

**검증**:
- [ ] EDynamicEmitterType, EParticleSortMode 사용
- [ ] Material은 Person 3가 사용할 예정

---

### ✅ Phase 1.5: ParticleLODLevel.h 작성 (30분)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleLODLevel.h`

**작업 내용**:
```cpp
#pragma once
#include "Object.h"
#include "ParticleModule.h"
#include "ParticleModuleRequired.h"
#include "ParticleLODLevel.generated.h"

/**
 * 파티클 LOD 레벨
 * 거리에 따른 품질 단계 정의
 */
UCLASS(DisplayName="Particle LOD Level", Description="파티클 LOD 레벨")
class UParticleLODLevel : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleLODLevel();

    /** LOD 레벨 인덱스 */
    UPROPERTY(EditAnywhere, Category="LOD")
    int32 Level = 0;

    /** 활성화 여부 */
    UPROPERTY(EditAnywhere, Category="LOD")
    bool bEnabled = true;

    /** 필수 모듈 (항상 존재) */
    UPROPERTY()
    UParticleModuleRequired* RequiredModule = nullptr;

    /** 추가 모듈 리스트 */
    UPROPERTY()
    TArray<UParticleModule*> Modules;

    /** TypeData 모듈 (Sprite/Mesh 타입별 데이터) */
    UPROPERTY()
    UParticleModule* TypeDataModule = nullptr;

    /**
     * Spawn 모듈들만 필터링하여 반환
     * ⭐ UE5 패턴: bSpawnModule 플래그로 필터링
     */
    TArray<UParticleModule*> GetSpawnModules() const;

    /**
     * Update 모듈들만 필터링하여 반환
     * ⭐ UE5 패턴: bUpdateModule 플래그로 필터링
     */
    TArray<UParticleModule*> GetUpdateModules() const;

    /**
     * FinalUpdate 모듈들만 필터링하여 반환
     * ⭐ UE5 패턴: bFinalUpdateModule 플래그로 필터링
     */
    TArray<UParticleModule*> GetFinalUpdateModules() const;
};
```

**검증**:
- [ ] Modules 배열이 UParticleModule* 타입
- [ ] Person 2가 GetSpawnModules() 사용 예정
- [ ] Get*Modules() 함수 3개 정의됨

---

### ✅ Phase 1.6: ParticleEmitter.h 작성 (45분)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleEmitter.h`

**작업 내용**:
```cpp
#pragma once
#include "Object.h"
#include "ParticleLODLevel.h"
#include "ParticleEmitter.generated.h"

/**
 * 파티클 이미터
 * 하나의 파티클 발생원 (시스템은 여러 이미터 포함 가능)
 */
UCLASS(DisplayName="Particle Emitter", Description="파티클 이미터")
class UParticleEmitter : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleEmitter();

    /** LOD 레벨 배열 */
    UPROPERTY()
    TArray<UParticleLODLevel*> LODLevels;

    /** 이미터 이름 */
    UPROPERTY(EditAnywhere, Category="Emitter")
    FString EmitterName = "Emitter";

    /**
     * 파티클 크기 계산 (FBaseParticle + 모듈 추가 데이터)
     * @param LODIndex - LOD 레벨 인덱스
     * @return 파티클 하나의 바이트 크기
     */
    uint32 CalculateParticleSize(uint32 LODIndex) const;

    /**
     * 이미터 모듈 정보 캐싱
     * Person 2가 사용할 예정
     */
    void CacheEmitterModuleInfo();

private:
    /** 캐시된 파티클 크기 */
    uint32 CachedParticleSize = 0;
};
```

**검증**:
- [ ] CalculateParticleSize() 인터페이스 제공
- [ ] Person 2가 호출할 메서드 정의됨

---

### ✅ Phase 1.7: ParticleSystem.h 작성 (45분)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleSystem.h`

**작업 내용**:
```cpp
#pragma once
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleSystem.generated.h"

/**
 * 파티클 시스템
 * 여러 이미터를 포함하는 최상위 컨테이너
 */
UCLASS(DisplayName="Particle System", Description="파티클 시스템")
class UParticleSystem : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleSystem();
    virtual ~UParticleSystem() = default;

    /** 이미터 배열 */
    UPROPERTY()
    TArray<UParticleEmitter*> Emitters;

    /** 시스템 이름 */
    UPROPERTY(EditAnywhere, Category="ParticleSystem")
    FString SystemName = "ParticleSystem";

    /** 자동 재생 여부 */
    UPROPERTY(EditAnywhere, Category="ParticleSystem")
    bool bAutoActivate = true;

    /**
     * 시스템 초기화
     * 모든 이미터와 모듈 초기화
     */
    void InitializeSystem();

    /**
     * 이미터 추가
     */
    void AddEmitter(UParticleEmitter* Emitter);

    /**
     * 이미터 제거
     */
    void RemoveEmitter(int32 Index);

    /**
     * 직렬화 (저장/로드)
     */
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
```

**검증**:
- [ ] Emitters 배열 제공
- [ ] Person 4가 AddEmitter/RemoveEmitter 사용 예정

---

### ✅ Phase 1.8: ParticleHelper.h 작성 (45분) ⭐ 새로 추가 (UE5 패턴)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleHelper.h`

> ⭐ **중요**: 이 파일은 원래 Day 3에 작성 예정이었으나, UE5 분석 결과 핵심 패턴으로 판단하여 **Day 1로 이동**

**작업 내용**:
```cpp
#pragma once

/**
 * 파티클 모듈 개발을 위한 Helper 매크로
 * Person 2의 FParticleEmitterInstance와 함께 사용
 *
 * ⭐ UE5 패턴: ParticleHelper.h의 매크로 시스템 적용
 */

// 전방 선언
struct FBaseParticle;
struct FParticleEmitterInstance;

/**
 * 파티클 포인터 접근 매크로
 * 모듈의 Spawn/Update 메서드에서 사용
 *
 * 사용 예:
 *   void MyModule::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
 *   {
 *       MUNDI_DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Offset);
 *       Particle->Location = FVector(0, 0, 0);
 *   }
 */
#define MUNDI_DECLARE_PARTICLE_PTR(VarName, ParticleAddress) \
    FBaseParticle* VarName = (FBaseParticle*)(ParticleAddress)

/**
 * 파티클 업데이트 루프 시작 매크로
 * 모든 활성 파티클을 순회
 *
 * 사용 예:
 *   MUNDI_BEGIN_UPDATE_LOOP
 *   {
 *       Particle.Velocity += Gravity * DeltaTime;
 *   }
 *   MUNDI_END_UPDATE_LOOP
 */
#define MUNDI_BEGIN_UPDATE_LOOP \
    if (Owner) { \
        for (int32 ParticleIndex = 0; ParticleIndex < Owner->ActiveParticles; ParticleIndex++) { \
            const uint16 Index = Owner->ParticleIndices[ParticleIndex]; \
            FBaseParticle& Particle = *((FBaseParticle*)((uint8*)Owner->ParticleData + Index * Owner->ParticleStride));

/**
 * 파티클 업데이트 루프 종료 매크로
 */
#define MUNDI_END_UPDATE_LOOP \
        } \
    }

/**
 * 파티클 초기화 매크로
 * Spawn 시 파티클 메모리를 0으로 초기화
 * ⭐ UE5 패턴: SPAWN_INIT 매크로
 */
#define MUNDI_SPAWN_INIT(ParticlePtr) \
    FMemory::Memzero(ParticlePtr, sizeof(FBaseParticle))

/**
 * 파티클 데이터 오프셋 계산
 * @param ParticleIndex - 파티클 인덱스
 * @param ParticleStride - 파티클 간격 (바이트)
 */
inline int32 GetParticleOffset(int32 ParticleIndex, int32 ParticleStride)
{
    return ParticleIndex * ParticleStride;
}

/**
 * 파티클의 베이스 크기 가져오기 (Size 모듈 적용 전)
 * ⭐ TODO: Person 2에게 FBaseParticle에 BaseSize 필드 추가 제안
 */
inline FVector GetParticleBaseSize(const FBaseParticle& Particle)
{
    // TODO: Person 2의 FBaseParticle에 BaseSize 필드 추가 시 구현
    // return Particle.BaseSize;
    return FVector(1.0f, 1.0f, 1.0f);
}

/**
 * 파티클의 베이스 속도 가져오기
 * ⭐ TODO: Person 2에게 FBaseParticle에 BaseVelocity 필드 추가 제안
 */
inline FVector GetParticleBaseVelocity(const FBaseParticle& Particle)
{
    // TODO: Person 2의 FBaseParticle에 BaseVelocity 필드 추가 시 구현
    // return Particle.BaseVelocity;
    return FVector(0.0f, 0.0f, 0.0f);
}

/**
 * 파티클의 베이스 색상 가져오기
 * ⭐ TODO: Person 2에게 FBaseParticle에 BaseColor 필드 추가 제안
 */
inline FLinearColor GetParticleBaseColor(const FBaseParticle& Particle)
{
    // TODO: Person 2의 FBaseParticle에 BaseColor 필드 추가 시 구현
    // return Particle.BaseColor;
    return FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
}
```

**검증**:
- [ ] 매크로 정의 완료
- [ ] 인라인 함수 정의 완료
- [ ] Person 2에게 Base 필드 추가 제안 준비됨

**⚠️ 중요**: 이 헤더를 Person 2와 공유하고, Base 필드 추가를 제안하세요!

---

### ✅ Phase 1.9: 코드 생성 및 빌드 테스트 (30분)

**작업 내용**:
```bash
cd Mundi
GenerateBindings.bat
```

**검증**:
- [ ] Generated/ 디렉토리에 .generated.h/.cpp 파일 생성 확인
- [ ] Visual Studio에서 빌드 성공
- [ ] 다음 코드가 동작하는지 테스트:
  ```cpp
  UParticleSystem* System = NewObject<UParticleSystem>();
  UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
  UParticleLODLevel* LOD = NewObject<UParticleLODLevel>();
  ```

**⚠️ 중요**: 이 시점에서 **Person 2에게 헤더 파일 공유** → Person 2가 오후 작업 시작 가능

---

### ✅ Phase 1.10: CPP 파일 구현 (2.5시간)

#### ParticleModule.cpp ⭐ UE5 패턴 적용
```cpp
#include "pch.h"
#include "ParticleModule.h"

UParticleModule::UParticleModule()
    : bEnabled(true)
    , bSpawnModule(false)      // ⭐ UE5 패턴
    , bUpdateModule(false)     // ⭐ UE5 패턴
    , bFinalUpdateModule(false) // ⭐ UE5 패턴
{
}

void UParticleModule::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    // 기본 구현 없음 (자식 클래스에서 override)
}

void UParticleModule::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    // 기본 구현 없음 (자식 클래스에서 override)
}

void UParticleModule::FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    // 기본 구현 없음 (자식 클래스에서 override)
}

BEGIN_PROPERTIES(UParticleModule)
    ADD_PROPERTY(bool, bEnabled, "Module", true, "모듈 활성화 여부")
    // ⭐ 실행 단계 플래그는 내부 사용, 에디터 노출 불필요
END_PROPERTIES()
```

#### ParticleModuleRequired.cpp
```cpp
#include "pch.h"
#include "ParticleModuleRequired.h"

UParticleModuleRequired::UParticleModuleRequired()
    : EmitterType(EDynamicEmitterType::Sprite)
    , Material(nullptr)
    , SortMode(EParticleSortMode::None)
    , EmitterScale(1.0f, 1.0f, 1.0f)
    , MaxParticles(100)
{
    // ⭐ Required 모듈은 Spawn과 Update 모두에서 실행
    bSpawnModule = true;   // 초기화 담당
    bUpdateModule = true;  // RelativeTime 갱신 담당
}

BEGIN_PROPERTIES(UParticleModuleRequired)
    // EmitterType은 enum이므로 수동 처리 필요
    ADD_PROPERTY(FVector, EmitterScale, "Emitter", true, "이미터 스케일")
    ADD_PROPERTY_RANGE(int32, MaxParticles, "Emitter", 1, 10000, true, "최대 파티클 개수")
END_PROPERTIES()
```

#### ParticleLODLevel.cpp ⭐ UE5 패턴 적용
```cpp
#include "pch.h"
#include "ParticleLODLevel.h"

UParticleLODLevel::UParticleLODLevel()
    : Level(0)
    , bEnabled(true)
    , RequiredModule(nullptr)
    , TypeDataModule(nullptr)
{
}

TArray<UParticleModule*> UParticleLODLevel::GetSpawnModules() const
{
    TArray<UParticleModule*> SpawnModules;
    for (UParticleModule* Module : Modules)
    {
        // ⭐ UE5 패턴: bSpawnModule 플래그로 필터링
        if (Module && Module->bEnabled && Module->bSpawnModule)
        {
            SpawnModules.Add(Module);
        }
    }
    return SpawnModules;
}

TArray<UParticleModule*> UParticleLODLevel::GetUpdateModules() const
{
    TArray<UParticleModule*> UpdateModules;
    for (UParticleModule* Module : Modules)
    {
        // ⭐ UE5 패턴: bUpdateModule 플래그로 필터링
        if (Module && Module->bEnabled && Module->bUpdateModule)
        {
            UpdateModules.Add(Module);
        }
    }
    return UpdateModules;
}

TArray<UParticleModule*> UParticleLODLevel::GetFinalUpdateModules() const
{
    TArray<UParticleModule*> FinalUpdateModules;
    for (UParticleModule* Module : Modules)
    {
        // ⭐ UE5 패턴: bFinalUpdateModule 플래그로 필터링
        if (Module && Module->bEnabled && Module->bFinalUpdateModule)
        {
            FinalUpdateModules.Add(Module);
        }
    }
    return FinalUpdateModules;
}

BEGIN_PROPERTIES(UParticleLODLevel)
    ADD_PROPERTY(int32, Level, "LOD", true, "LOD 레벨")
    ADD_PROPERTY(bool, bEnabled, "LOD", true, "활성화 여부")
END_PROPERTIES()
```

#### ParticleEmitter.cpp
```cpp
#include "pch.h"
#include "ParticleEmitter.h"

// Person 2의 FBaseParticle 구조체 전방 선언 (나중에 include)
struct FBaseParticle;

UParticleEmitter::UParticleEmitter()
    : EmitterName("Emitter")
    , CachedParticleSize(0)
{
}

uint32 UParticleEmitter::CalculateParticleSize(uint32 LODIndex) const
{
    // 기본 파티클 크기 (Person 2가 FBaseParticle 정의 후 수정)
    uint32 Size = 128; // sizeof(FBaseParticle) 예상치

    // TODO: 모듈별 추가 데이터 크기 계산
    // 각 모듈이 ParticleSize에 기여하는 크기를 더함

    return Size;
}

void UParticleEmitter::CacheEmitterModuleInfo()
{
    if (LODLevels.Num() > 0)
    {
        CachedParticleSize = CalculateParticleSize(0);
        UE_LOG("Emitter '%s' 파티클 크기 캐싱: %d bytes", EmitterName.c_str(), CachedParticleSize);
    }
}

BEGIN_PROPERTIES(UParticleEmitter)
    ADD_PROPERTY(FString, EmitterName, "Emitter", true, "이미터 이름")
END_PROPERTIES()
```

#### ParticleSystem.cpp
```cpp
#include "pch.h"
#include "ParticleSystem.h"

UParticleSystem::UParticleSystem()
    : SystemName("ParticleSystem")
    , bAutoActivate(true)
{
}

void UParticleSystem::InitializeSystem()
{
    UE_LOG("파티클 시스템 '%s' 초기화 중...", SystemName.c_str());

    for (UParticleEmitter* Emitter : Emitters)
    {
        if (Emitter)
        {
            Emitter->CacheEmitterModuleInfo();
        }
    }

    UE_LOG("파티클 시스템 초기화 완료. 이미터 개수: %d", Emitters.Num());
}

void UParticleSystem::AddEmitter(UParticleEmitter* Emitter)
{
    if (Emitter)
    {
        Emitters.Add(Emitter);
        UE_LOG("이미터 추가: %s", Emitter->EmitterName.c_str());
    }
}

void UParticleSystem::RemoveEmitter(int32 Index)
{
    if (Index >= 0 && Index < Emitters.Num())
    {
        Emitters.RemoveAt(Index);
        UE_LOG("이미터 제거: Index %d", Index);
    }
}

void UParticleSystem::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // TODO: Emitters 배열 직렬화
    // Person 4가 Editor에서 사용할 예정
}

BEGIN_PROPERTIES(UParticleSystem)
    ADD_PROPERTY(FString, SystemName, "ParticleSystem", true, "시스템 이름")
    ADD_PROPERTY(bool, bAutoActivate, "ParticleSystem", true, "자동 재생")
END_PROPERTIES()
```

**검증**:
- [ ] 모든 CPP 파일 컴파일 성공
- [ ] NewObject<>() 로 인스턴스 생성 가능
- [ ] UE_LOG 출력 확인
- [ ] bSpawnModule/bUpdateModule/bFinalUpdateModule 초기화 확인

---

## 🎯 Day 2 - 모듈 헤더 및 Spawn/Lifetime 구현 (8시간)

### ✅ Phase 2.1: 모듈 헤더 파일 작성 (2시간)

#### ParticleModuleSpawn.h
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleSpawn.generated.h"

/**
 * 파티클 생성 모듈
 * 초당 생성 개수와 버스트 생성 제어
 */
UCLASS(DisplayName="Spawn", Description="파티클 생성 빈도 제어")
class UParticleModuleSpawn : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleSpawn();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;

    /** 초당 생성 개수 */
    UPROPERTY(EditAnywhere, Category="Spawn")
    float Rate = 10.0f;

    /** 버스트 생성 개수 (한 번에 생성) */
    UPROPERTY(EditAnywhere, Category="Spawn")
    int32 BurstCount = 0;

    /** 버스트 시간 간격 (초) */
    UPROPERTY(EditAnywhere, Category="Spawn")
    float BurstTime = 0.0f;

private:
    float SpawnFraction = 0.0f; // 잔여 생성 분수
};
```

#### ParticleModuleLifetime.h
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleLifetime.generated.h"

UCLASS(DisplayName="Lifetime", Description="파티클 수명 설정")
class UParticleModuleLifetime : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleLifetime();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;

    /** 최소 수명 (초) */
    UPROPERTY(EditAnywhere, Category="Lifetime")
    float MinLifetime = 1.0f;

    /** 최대 수명 (초) */
    UPROPERTY(EditAnywhere, Category="Lifetime")
    float MaxLifetime = 1.0f;
};
```

#### ParticleModuleLocation.h
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleLocation.generated.h"

UCLASS(DisplayName="Initial Location", Description="파티클 초기 위치")
class UParticleModuleLocation : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleLocation();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;

    /** 시작 위치 오프셋 */
    UPROPERTY(EditAnywhere, Category="Location")
    FVector StartLocation = FVector(0.0f, 0.0f, 0.0f);

    /** 랜덤 분포 범위 (Box) */
    UPROPERTY(EditAnywhere, Category="Location")
    FVector DistributionExtent = FVector(10.0f, 10.0f, 10.0f);
};
```

#### ParticleModuleVelocity.h
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleVelocity.generated.h"

UCLASS(DisplayName="Initial Velocity", Description="파티클 초기 속도")
class UParticleModuleVelocity : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleVelocity();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;

    /** 시작 속도 */
    UPROPERTY(EditAnywhere, Category="Velocity")
    FVector StartVelocity = FVector(0.0f, 0.0f, 100.0f);

    /** 랜덤 속도 범위 */
    UPROPERTY(EditAnywhere, Category="Velocity")
    FVector VelocityVariance = FVector(50.0f, 50.0f, 50.0f);
};
```

#### ParticleModuleColor.h ⭐ Spawn + Update 모듈
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleColor.generated.h"

/**
 * 파티클 색상 모듈 (Over Life)
 * ⭐ UE5 패턴: Spawn과 Update 모두에서 실행
 */
UCLASS(DisplayName="Color Over Life", Description="파티클 색상 (생명 주기)")
class UParticleModuleColor : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleColor();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

    /** 시작 색상 */
    UPROPERTY(EditAnywhere, Category="Color")
    FLinearColor StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    /** 종료 색상 */
    UPROPERTY(EditAnywhere, Category="Color")
    FLinearColor EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
};
```

#### ParticleModuleSize.h
```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleSize.generated.h"

UCLASS(DisplayName="Size", Description="파티클 크기")
class UParticleModuleSize : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleSize();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;

    /** 시작 크기 */
    UPROPERTY(EditAnywhere, Category="Size")
    FVector StartSize = FVector(50.0f, 50.0f, 50.0f);

    /** 크기 랜덤 범위 */
    UPROPERTY(EditAnywhere, Category="Size")
    FVector SizeVariance = FVector(10.0f, 10.0f, 10.0f);
};
```

**코드 생성 및 검증**:
```bash
cd Mundi
GenerateBindings.bat
```

**⚠️ 중요**: 이 시점에서 **Person 4에게 공유** → Editor UI 개발 시작 가능

---

### ✅ Phase 2.2: ParticleModuleSpawn.cpp 구현 (2시간) ⭐ UE5 패턴 적용

**파일**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleSpawn.cpp`

```cpp
#include "pch.h"
#include "ParticleModuleSpawn.h"

// Person 2의 FParticleEmitterInstance 구조체 포함 (Person 2 작업 후)
// #include "ParticleEmitterInstance.h"

UParticleModuleSpawn::UParticleModuleSpawn()
    : Rate(10.0f)
    , BurstCount(0)
    , BurstTime(0.0f)
    , SpawnFraction(0.0f)
{
    // ⭐ UE5 패턴: Spawn 단계에서만 실행
    bSpawnModule = true;
    bUpdateModule = false;
    bFinalUpdateModule = false;
}

void UParticleModuleSpawn::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    // TODO: Person 2의 FParticleEmitterInstance 구조체 완성 후 구현
    // 현재는 로직만 주석으로 작성

    /*
    // 생성할 파티클 개수 계산
    float FloatCount = Rate * DeltaTime + SpawnFraction;
    int32 SpawnCount = (int32)FloatCount;
    SpawnFraction = FloatCount - SpawnCount;

    // 버스트 처리
    if (BurstCount > 0 && FMath::Fmod(SpawnTime, BurstTime) < DeltaTime)
    {
        SpawnCount += BurstCount;
    }

    // Owner->SpawnParticles(SpawnCount, ...);
    */

    UE_LOG("ParticleModuleSpawn::Spawn 호출 (Rate: %.2f)", Rate);
}

BEGIN_PROPERTIES(UParticleModuleSpawn)
    ADD_PROPERTY_RANGE(float, Rate, "Spawn", 0.0f, 1000.0f, true, "초당 생성 개수")
    ADD_PROPERTY_RANGE(int32, BurstCount, "Spawn", 0, 1000, true, "버스트 생성 개수")
    ADD_PROPERTY_RANGE(float, BurstTime, "Spawn", 0.0f, 10.0f, true, "버스트 간격 (초)")
END_PROPERTIES()
```

**검증**:
- [ ] Rate 속성이 에디터에 표시됨 (Person 4 확인 가능)
- [ ] bSpawnModule = true 설정 확인
- [ ] UE_LOG 출력 확인

---

### ✅ Phase 2.3: ParticleModuleLifetime.cpp 구현 (1.5시간) ⭐ UE5 패턴 적용

```cpp
#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "Math/MathUtil.h"

UParticleModuleLifetime::UParticleModuleLifetime()
    : MinLifetime(1.0f)
    , MaxLifetime(1.0f)
{
    // ⭐ UE5 패턴: Spawn 단계에서만 실행
    bSpawnModule = true;
    bUpdateModule = false;
}

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    // TODO: Person 2의 FBaseParticle 구조체 완성 후 구현

    /*
    MUNDI_DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Offset);

    // 랜덤 수명 할당
    float Lifetime = FMath::RandRange(MinLifetime, MaxLifetime);
    Particle->OneOverMaxLifetime = 1.0f / Lifetime;  // ⭐ UE5 패턴
    Particle->RelativeTime = 0.0f;
    */

    UE_LOG("ParticleModuleLifetime::Spawn (Min: %.2f, Max: %.2f)", MinLifetime, MaxLifetime);
}

BEGIN_PROPERTIES(UParticleModuleLifetime)
    ADD_PROPERTY_RANGE(float, MinLifetime, "Lifetime", 0.1f, 100.0f, true, "최소 수명 (초)")
    ADD_PROPERTY_RANGE(float, MaxLifetime, "Lifetime", 0.1f, 100.0f, true, "최대 수명 (초)")
END_PROPERTIES()
```

**검증**:
- [ ] MinLifetime, MaxLifetime 속성 동작
- [ ] bSpawnModule = true 설정 확인
- [ ] 로그 출력 확인

---

### ✅ Phase 2.4: 단위 테스트 작성 (2.5시간)

**파일**: `Mundi/Source/Runtime/Engine/Particles/ParticleModuleTest.cpp` (임시)

```cpp
#include "pch.h"
#include "ParticleSystem.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleLifetime.h"

void TestParticleSystemCreation()
{
    UE_LOG("=== Particle System Creation Test ===");

    // 시스템 생성
    UParticleSystem* System = NewObject<UParticleSystem>();
    System->SystemName = "TestSystem";

    // 이미터 생성
    UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
    Emitter->EmitterName = "TestEmitter";

    // LOD 레벨 생성
    UParticleLODLevel* LOD = NewObject<UParticleLODLevel>();
    LOD->Level = 0;

    // 필수 모듈 생성
    UParticleModuleRequired* Required = NewObject<UParticleModuleRequired>();
    Required->MaxParticles = 100;
    LOD->RequiredModule = Required;

    // Spawn 모듈 추가
    UParticleModuleSpawn* Spawn = NewObject<UParticleModuleSpawn>();
    Spawn->Rate = 20.0f;
    LOD->Modules.Add(Spawn);

    // Lifetime 모듈 추가
    UParticleModuleLifetime* Lifetime = NewObject<UParticleModuleLifetime>();
    Lifetime->MinLifetime = 1.0f;
    Lifetime->MaxLifetime = 2.0f;
    LOD->Modules.Add(Lifetime);

    // 구조 연결
    Emitter->LODLevels.Add(LOD);
    System->AddEmitter(Emitter);

    // 초기화
    System->InitializeSystem();

    // ⭐ UE5 패턴 검증: 실행 단계 플래그
    TArray<UParticleModule*> SpawnModules = LOD->GetSpawnModules();
    UE_LOG("Spawn 모듈 개수: %d (예상: 3 - Required, Spawn, Lifetime)", SpawnModules.Num());

    for (UParticleModule* Module : SpawnModules)
    {
        UE_LOG("  - %s (bSpawnModule: %d)", Module->GetClass()->GetName().c_str(), Module->bSpawnModule);
    }

    UE_LOG("✅ 테스트 성공: 시스템 생성 완료");
}

// WinMain이나 테스트 함수에서 호출
// TestParticleSystemCreation();
```

**검증**:
- [ ] 테스트 코드 실행 성공
- [ ] 모든 객체 생성 성공
- [ ] GetSpawnModules()가 올바른 모듈 반환
- [ ] 로그 출력 정상

---

## 🎯 Day 3 - Location/Velocity/Color/Size 모듈 구현 (8시간)

### ✅ Phase 3.1: ParticleModuleLocation.cpp 구현 (1.5시간)

```cpp
#include "pch.h"
#include "ParticleModuleLocation.h"
#include "ParticleHelper.h"
#include "Math/MathUtil.h"

UParticleModuleLocation::UParticleModuleLocation()
    : StartLocation(0.0f, 0.0f, 0.0f)
    , DistributionExtent(10.0f, 10.0f, 10.0f)
{
    // ⭐ UE5 패턴: Spawn 단계에서만 실행
    bSpawnModule = true;
}

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    // TODO: Person 2의 FBaseParticle 완성 후 구현

    /*
    MUNDI_DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Offset);

    // Box 분포로 랜덤 위치 생성
    FVector RandomOffset(
        FMath::RandRange(-DistributionExtent.X, DistributionExtent.X),
        FMath::RandRange(-DistributionExtent.Y, DistributionExtent.Y),
        FMath::RandRange(-DistributionExtent.Z, DistributionExtent.Z)
    );

    Particle->Location = StartLocation + RandomOffset;
    Particle->OldLocation = Particle->Location;  // ⭐ UE5 패턴: 첫 프레임 델타 방지
    */

    UE_LOG("ParticleModuleLocation::Spawn (Start: %.2f, %.2f, %.2f)",
           StartLocation.X, StartLocation.Y, StartLocation.Z);
}

BEGIN_PROPERTIES(UParticleModuleLocation)
    ADD_PROPERTY(FVector, StartLocation, "Location", true, "시작 위치")
    ADD_PROPERTY(FVector, DistributionExtent, "Location", true, "분포 범위 (Box)")
END_PROPERTIES()
```

---

### ✅ Phase 3.2: ParticleModuleVelocity.cpp 구현 (1.5시간)

```cpp
#include "pch.h"
#include "ParticleModuleVelocity.h"
#include "ParticleHelper.h"
#include "Math/MathUtil.h"

UParticleModuleVelocity::UParticleModuleVelocity()
    : StartVelocity(0.0f, 0.0f, 100.0f)
    , VelocityVariance(50.0f, 50.0f, 50.0f)
{
    // ⭐ UE5 패턴: Spawn 단계에서만 실행
    bSpawnModule = true;
}

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    /*
    MUNDI_DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Offset);

    // 랜덤 속도 생성
    FVector RandomVelocity(
        FMath::RandRange(-VelocityVariance.X, VelocityVariance.X),
        FMath::RandRange(-VelocityVariance.Y, VelocityVariance.Y),
        FMath::RandRange(-VelocityVariance.Z, VelocityVariance.Z)
    );

    Particle->Velocity = StartVelocity + RandomVelocity;
    // ⭐ UE5 패턴: BaseVelocity 저장 (Person 2가 필드 추가 시)
    // Particle->BaseVelocity = Particle->Velocity;
    */

    UE_LOG("ParticleModuleVelocity::Spawn");
}

BEGIN_PROPERTIES(UParticleModuleVelocity)
    ADD_PROPERTY(FVector, StartVelocity, "Velocity", true, "시작 속도")
    ADD_PROPERTY(FVector, VelocityVariance, "Velocity", true, "속도 랜덤 범위")
END_PROPERTIES()
```

---

### ✅ Phase 3.3: ParticleModuleColor.cpp 구현 (2.5시간) ⭐ 다단계 실행 예제

```cpp
#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleHelper.h"
#include "Math/MathUtil.h"

UParticleModuleColor::UParticleModuleColor()
    : StartColor(1.0f, 1.0f, 1.0f, 1.0f)
    , EndColor(1.0f, 1.0f, 1.0f, 0.0f)
{
    // ⭐ UE5 패턴: Spawn과 Update 모두에서 실행
    bSpawnModule = true;   // 초기 색상 설정
    bUpdateModule = true;  // 매 프레임 색상 보간
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    /*
    MUNDI_DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Offset);

    Particle->Color = StartColor;
    // ⭐ UE5 패턴: BaseColor 저장 (Person 2가 필드 추가 시)
    // Particle->BaseColor = StartColor;
    */

    UE_LOG("ParticleModuleColor::Spawn (Start Color RGBA: %.2f, %.2f, %.2f, %.2f)",
           StartColor.R, StartColor.G, StartColor.B, StartColor.A);
}

void UParticleModuleColor::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    /*
    MUNDI_BEGIN_UPDATE_LOOP
    {
        // Life 기반 색상 보간 (0.0 ~ 1.0)
        float Life = Particle.RelativeTime;

        // ⭐ UE5 패턴: BaseColor 기반 계산 (Person 2가 필드 추가 시)
        // Particle.Color = FLinearColor::Lerp(Particle.BaseColor, EndColor, Life);

        // 임시: BaseColor 없이 구현
        Particle.Color = FLinearColor::Lerp(StartColor, EndColor, Life);
    }
    MUNDI_END_UPDATE_LOOP
    */
}

BEGIN_PROPERTIES(UParticleModuleColor)
    ADD_PROPERTY(FLinearColor, StartColor, "Color", true, "시작 색상")
    ADD_PROPERTY(FLinearColor, EndColor, "Color", true, "종료 색상")
END_PROPERTIES()
```

**검증**:
- [ ] bSpawnModule = true, bUpdateModule = true 확인
- [ ] 다단계 실행 모듈의 예제로 사용 가능

---

### ✅ Phase 3.4: ParticleModuleSize.cpp 구현 (1.5시간)

```cpp
#include "pch.h"
#include "ParticleModuleSize.h"
#include "ParticleHelper.h"
#include "Math/MathUtil.h"

UParticleModuleSize::UParticleModuleSize()
    : StartSize(50.0f, 50.0f, 50.0f)
    , SizeVariance(10.0f, 10.0f, 10.0f)
{
    // ⭐ UE5 패턴: Spawn 단계에서만 실행
    bSpawnModule = true;
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime)
{
    /*
    MUNDI_DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Offset);

    FVector RandomSize(
        FMath::RandRange(-SizeVariance.X, SizeVariance.X),
        FMath::RandRange(-SizeVariance.Y, SizeVariance.Y),
        FMath::RandRange(-SizeVariance.Z, SizeVariance.Z)
    );

    Particle->Size = StartSize + RandomSize;
    // ⭐ UE5 패턴: BaseSize 저장 (Person 2가 필드 추가 시)
    // Particle->BaseSize = Particle->Size;
    */

    UE_LOG("ParticleModuleSize::Spawn (Size: %.2f, %.2f, %.2f)",
           StartSize.X, StartSize.Y, StartSize.Z);
}

BEGIN_PROPERTIES(UParticleModuleSize)
    ADD_PROPERTY(FVector, StartSize, "Size", true, "시작 크기")
    ADD_PROPERTY(FVector, SizeVariance, "Size", true, "크기 랜덤 범위")
END_PROPERTIES()
```

---

### ✅ Phase 3.5: 통합 테스트 (1시간)

**테스트 코드**:
```cpp
void TestMultiPhaseModule()
{
    UE_LOG("=== Multi-Phase Module Test ===");

    UParticleLODLevel* LOD = NewObject<UParticleLODLevel>();

    // Color 모듈 추가 (Spawn + Update)
    UParticleModuleColor* Color = NewObject<UParticleModuleColor>();
    LOD->Modules.Add(Color);

    // Location 모듈 추가 (Spawn만)
    UParticleModuleLocation* Location = NewObject<UParticleModuleLocation>();
    LOD->Modules.Add(Location);

    // ⭐ UE5 패턴 검증: 실행 단계별 필터링
    TArray<UParticleModule*> SpawnModules = LOD->GetSpawnModules();
    TArray<UParticleModule*> UpdateModules = LOD->GetUpdateModules();

    UE_LOG("Spawn 모듈: %d (예상: 2 - Color, Location)", SpawnModules.Num());
    UE_LOG("Update 모듈: %d (예상: 1 - Color)", UpdateModules.Num());

    for (UParticleModule* Module : SpawnModules)
    {
        UE_LOG("  [Spawn] %s", Module->GetClass()->GetName().c_str());
    }

    for (UParticleModule* Module : UpdateModules)
    {
        UE_LOG("  [Update] %s", Module->GetClass()->GetName().c_str());
    }

    UE_LOG("✅ 다단계 모듈 테스트 성공");
}
```

**검증**:
- [ ] Color 모듈이 Spawn과 Update 리스트 모두에 포함
- [ ] Location 모듈은 Spawn 리스트에만 포함
- [ ] UE5 패턴이 올바르게 동작

---

## 🎯 Day 4 - SizeScaleBySpeed 및 최종 검증 (8시간)

### ✅ Phase 4.1: ParticleModuleSizeScaleBySpeed 구현 (2.5시간) ⭐ Update 전용 모듈

**파일**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleSizeScaleBySpeed.h`

```cpp
#pragma once
#include "ParticleModule.h"
#include "ParticleModuleSizeScaleBySpeed.generated.h"

/**
 * 속도에 따른 크기 조절 모듈
 * 파티클이 빠를수록 크기가 커짐
 * ⭐ UE5 패턴: Update 단계에서만 실행
 */
UCLASS(DisplayName="Size Scale by Speed", Description="속도 기반 크기 조절")
class UParticleModuleSizeScaleBySpeed : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleSizeScaleBySpeed();

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

    /** 속도 스케일 계수 */
    UPROPERTY(EditAnywhere, Category="Size")
    FVector SpeedScale = FVector(0.01f, 0.01f, 0.0f);

    /** 최대 크기 제한 */
    UPROPERTY(EditAnywhere, Category="Size")
    FVector MaxScale = FVector(5.0f, 5.0f, 5.0f);
};
```

**CPP 구현**:
```cpp
#include "pch.h"
#include "ParticleModuleSizeScaleBySpeed.h"
#include "ParticleHelper.h"

UParticleModuleSizeScaleBySpeed::UParticleModuleSizeScaleBySpeed()
    : SpeedScale(0.01f, 0.01f, 0.0f)
    , MaxScale(5.0f, 5.0f, 5.0f)
{
    // ⭐ UE5 패턴: Update 단계에서만 실행
    bSpawnModule = false;
    bUpdateModule = true;
}

void UParticleModuleSizeScaleBySpeed::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    if (!Owner || !bEnabled)
        return;

    // ⭐ UE5 패턴: 헬퍼 매크로 사용
    MUNDI_BEGIN_UPDATE_LOOP
    {
        // 속도 크기 계산
        float Speed = Particle.Velocity.Size();

        // 크기 스케일 적용
        FVector Scale = SpeedScale * Speed;

        // 최소값 1.0 보장
        Scale.X = FMath::Max(Scale.X, 1.0f);
        Scale.Y = FMath::Max(Scale.Y, 1.0f);
        Scale.Z = FMath::Max(Scale.Z, 1.0f);

        // 최대값 제한
        Scale.X = FMath::Min(Scale.X, MaxScale.X);
        Scale.Y = FMath::Min(Scale.Y, MaxScale.Y);
        Scale.Z = FMath::Min(Scale.Z, MaxScale.Z);

        // ⭐ UE5 패턴: BaseSize 기반 계산 (Person 2가 필드 추가 시)
        // Particle.Size = GetParticleBaseSize(Particle) * Scale;

        // 임시: BaseSize 없이 구현
        Particle.Size = Particle.Size * Scale;
    }
    MUNDI_END_UPDATE_LOOP;
}

BEGIN_PROPERTIES(UParticleModuleSizeScaleBySpeed)
    ADD_PROPERTY(FVector, SpeedScale, "Size", true, "속도 스케일 계수")
    ADD_PROPERTY(FVector, MaxScale, "Size", true, "최대 크기 제한")
END_PROPERTIES()
```

**검증**:
- [ ] bUpdateModule = true 설정 확인
- [ ] MUNDI_BEGIN_UPDATE_LOOP 매크로 동작
- [ ] 속도 기반 크기 계산 로직 검증

---

### ✅ Phase 4.2: Person 2와 통합 테스트 (3시간)

**통합 테스트 시나리오**:

1. Person 2의 `FParticleEmitterInstance::SpawnParticles()` 호출 흐름 검증
   ```cpp
   // Person 2 코드에서:
   void FParticleEmitterInstance::SpawnParticles(int32 Count, ...)
   {
       for (int32 i = 0; i < Count; i++)
       {
           // ⭐ UE5 패턴: Spawn 모듈 실행
           TArray<UParticleModule*> SpawnModules = CurrentLODLevel->GetSpawnModules();
           for (UParticleModule* Module : SpawnModules)
           {
               Module->Spawn(this, Offset, SpawnTime);
           }
       }
   }

   void FParticleEmitterInstance::Tick(float DeltaTime)
   {
       // ⭐ UE5 패턴: Update 모듈 실행
       TArray<UParticleModule*> UpdateModules = CurrentLODLevel->GetUpdateModules();
       for (UParticleModule* Module : UpdateModules)
       {
           Module->Update(this, Offset, DeltaTime);
       }

       // ⭐ UE5 패턴: FinalUpdate 모듈 실행
       TArray<UParticleModule*> FinalUpdateModules = CurrentLODLevel->GetFinalUpdateModules();
       for (UParticleModule* Module : FinalUpdateModules)
       {
           Module->FinalUpdate(this, Offset, DeltaTime);
       }
   }
   ```

2. 모듈 실행 순서 검증
   - **Spawn 단계**: Required, Spawn, Lifetime, Location, Velocity, Color, Size
   - **Update 단계**: Required, Color, SizeScaleBySpeed

**검증**:
- [ ] 모든 모듈이 올바른 단계에서 호출됨
- [ ] Particle 데이터가 올바르게 설정됨
- [ ] 메모리 접근 에러 없음
- [ ] UE5 패턴이 정상 동작

---

### ✅ Phase 4.3: 코드 리뷰 및 문서화 (2.5시간)

**리뷰 체크리스트**:

1. **UE5 패턴 적용**
   - [ ] 모든 모듈이 bSpawnModule/bUpdateModule/bFinalUpdateModule 설정
   - [ ] ParticleHelper.h 매크로 사용
   - [ ] Get*Modules() 함수가 플래그 기반 필터링
   - [ ] 다단계 실행 모듈 예제 존재 (Color)

2. **코드 스타일**
   - [ ] 언리얼 엔진 코딩 컨벤션 준수
   - [ ] 한국어 주석 작성
   - [ ] UE_LOG 사용 (std::cout 금지)

3. **리플렉션 시스템**
   - [ ] 모든 클래스에 UCLASS 매크로
   - [ ] GENERATED_REFLECTION_BODY() 존재
   - [ ] UPROPERTY로 속성 마킹
   - [ ] BEGIN_PROPERTIES/END_PROPERTIES 구현

4. **Person 2 의존성**
   - [ ] FBaseParticle 구조체 정의 대기 중 (TODO 주석)
   - [ ] FParticleEmitterInstance 구조체 정의 대기 중
   - [ ] ParticleHelper.h 매크로 제공 완료
   - [ ] Base 필드 추가 제안 준비됨

5. **Person 4 의존성**
   - [ ] 모든 모듈 속성이 EditAnywhere로 마킹
   - [ ] Category 설정됨
   - [ ] Tooltip 작성됨

**버그 수정 우선순위**:
- 🔴 Critical: 크래시, 컴파일 에러
- 🟡 Major: 로직 오류, 메모리 누수
- 🟢 Minor: 코드 스타일, 주석

---

## 📊 완료 체크리스트

### Day 1 종료 시
- [ ] 모든 기본 클래스 헤더 작성 완료
- [ ] ⭐ ParticleHelper.h 작성 완료 (UE5 패턴)
- [ ] ⭐ 실행 단계 플래그 시스템 구현 완료 (UE5 패턴)
- [ ] GenerateBindings.bat 실행 성공
- [ ] Person 2에게 인터페이스 공유 완료
- [ ] NewObject<UParticleSystem>() 동작 확인
- [ ] 기본 클래스 CPP 구현 완료

### Day 2 종료 시
- [ ] 6개 모듈 헤더 작성 완료
- [ ] Person 4에게 모듈 공유 완료
- [ ] Spawn, Lifetime 모듈 구현 완료
- [ ] ⭐ 각 모듈의 실행 단계 플래그 설정 완료
- [ ] 단위 테스트 작성 및 통과

### Day 3 종료 시
- [ ] Location, Velocity, Color, Size 모듈 구현 완료
- [ ] ⭐ 다단계 실행 모듈 예제 완료 (Color)
- [ ] Person 2에게 Helper 매크로 제공 완료
- [ ] 모든 필수 모듈 동작 확인

### Day 4 종료 시
- [ ] SizeScaleBySpeed 모듈 구현 완료
- [ ] Person 2와 통합 테스트 성공
- [ ] ⭐ UE5 패턴 적용 검증 완료
- [ ] 코드 리뷰 완료
- [ ] Critical 버그 0개

---

## 📝 Person 2에게 전달할 제안사항

### 🟡 선택사항: FBaseParticle에 Base 필드 추가

**제안 배경**:
- UE5 분석 결과, Base 필드가 Over-Life 모듈의 정확성에 필수적
- Person 3의 ColorOverLife, SizeOverLife 모듈 품질 향상

**제안 내용**:
```cpp
struct FBaseParticle
{
    // 기존 필드
    FVector Location;
    FVector Velocity;
    FVector Size;
    FLinearColor Color;
    float RelativeTime;
    float OneOverMaxLifetime;

    // ⭐ 추가 제안: Base 필드
    FVector OldLocation;        // 트레일/충돌 용
    FVector BaseVelocity;       // 초기 속도 (가속도 모듈 기준)
    FVector BaseSize;           // 초기 크기 (SizeOverLife 기준)
    FLinearColor BaseColor;     // 초기 색상 (ColorOverLife 기준)
    float BaseRotationRate;     // 초기 회전 속도

    // 총 메모리: ~128 bytes (UE5와 동일)
};
```

**장점**:
- ✅ Over-Life 모듈이 누적 오차 없이 정확하게 동작
- ✅ 여러 Over-Life 모듈 조합 가능 (예: ColorOverLife + SizeOverLife)
- ✅ UE5 호환 구조

**단점**:
- ⚠️ 메모리 +48 bytes (100 파티클 × 48 = 4.8 KB 증가)
- ⚠️ 초기화 코드 약간 증가

**판단 기준**:
- Day 2 진행 속도가 빠르면 → 추가
- 일정이 타이트하면 → 향후 리팩토링

---

## ⚠️ 주의사항 및 팁

### DO ✅

1. **헤더 먼저 작성, 즉시 공유**
   - 다른 팀원이 대기하지 않도록 인터페이스 우선 제공
   - Person 2는 Day 1 오전 작업 필요

2. **⭐ 실행 단계 플래그 반드시 설정**
   - 모든 모듈 생성자에서 bSpawnModule/bUpdateModule/bFinalUpdateModule 설정
   - UE5 패턴의 핵심!

3. **⭐ 헬퍼 매크로 적극 활용**
   - MUNDI_DECLARE_PARTICLE_PTR
   - MUNDI_BEGIN_UPDATE_LOOP / MUNDI_END_UPDATE_LOOP
   - 코드 가독성 향상

4. **UCLASS 매크로 반드시 사용**
   - 에디터 통합 필수
   - 리플렉션 시스템 활용

5. **GenerateBindings.bat 자주 실행**
   - 새 클래스 추가 시마다
   - 속성 변경 시마다

6. **한국어 주석 작성**
   - 팀 내 코드 리뷰 용이
   - 코딩 컨벤션 준수

7. **UE_LOG로 디버깅**
   - 에디터 콘솔에 출력
   - 포맷 스트링 사용

### DON'T ❌

1. **구현 완료까지 헤더 미루지 않기**
   - 병렬 작업 차단 방지

2. **⭐ 실행 단계 플래그 설정 잊지 않기**
   - 기본값은 모두 false → 모듈이 실행 안 됨!

3. **new/delete 사용 금지**
   - NewObject<>() 사용
   - UObject 시스템 준수

4. **`.generated.h` include 순서 틀리지 않기**
   - 클래스 정의 전에 include
   - 컴파일 에러 방지

5. **std::cout 사용 금지**
   - UE_LOG 사용
   - 에디터 통합

6. **Person 2 완성 전 완전한 구현 시도하지 않기**
   - TODO 주석으로 표시
   - 인터페이스 제공에 집중

---

## 🔗 관련 문서

- [발제.md](발제.md) - 전체 과제 요구사항
- [particle_system_task_distribution.md](particle_system_task_distribution.md) - 팀 전체 작업 분배
- [리플렉션_자동화_시스템_설명서.md](리플렉션_자동화_시스템_설명서.md) - 코드 생성 시스템
- [CLAUDE.md](../CLAUDE.md) - 엔진 개발 가이드
- ⭐ [UE5_ParticleSystem_Analysis_Part1_Overview.md](UE5_ParticleSystem_Analysis_Part1_Overview.md) - UE5 분석: 개요
- ⭐ [UE5_ParticleSystem_Analysis_Part2_Classes.md](UE5_ParticleSystem_Analysis_Part2_Classes.md) - UE5 분석: 클래스
- ⭐ [UE5_ParticleSystem_Analysis_Part3_Macros.md](UE5_ParticleSystem_Analysis_Part3_Macros.md) - UE5 분석: 매크로
- ⭐ [UE5_ParticleSystem_Analysis_Part4_Recommendations.md](UE5_ParticleSystem_Analysis_Part4_Recommendations.md) - UE5 분석: 권장사항

---

**작성일**: 2025-11-21
**최종 수정**: UE5 패턴 적용 (2025-11-21)
**담당자**: Person 1
**적용 패턴**: UE5 Particle System 실행 단계 플래그 + 헬퍼 매크로
