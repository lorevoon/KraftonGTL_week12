# Day 2: 파티클 모듈 헤더 및 Spawn/Lifetime 구현 계획

## 📋 작업 개요

**목표**: 6개 파티클 모듈 헤더 작성 및 Spawn/Lifetime 모듈 CPP 구현
**완료 조건**: 컴파일 성공 및 기본 파티클 생성 가능

---

## 🔑 Mundi 호환성 패턴 (Day 1에서 학습)

### 필수 준수 사항

```cpp
// ✅ GOOD - Mundi 패턴
#include "../ParticleModule.h"              // Modules 서브디렉토리에서 상위 디렉토리 참조
#include "UParticleModuleSpawn.generated.h" // U prefix 필수

UCLASS(DisplayName="생성 모듈", Description="파티클 생성 위치를 제어합니다")
class UParticleModuleSpawn : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleSpawn();

    // Spawn 메서드: 4개 파라미터 (ParticleBase 포함)
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

    UPROPERTY(EditAnywhere, Category="Spawn")
    FVector LocationMin;  // 헤더에서 기본값 설정 금지
};

// Constructor: 초기화 리스트 사용
UParticleModuleSpawn::UParticleModuleSpawn()
    : LocationMin(FVector())  // FVector() 사용, FVector(0,0,0) 아님
    , LocationMax(FVector())
{
    bSpawnModule = true;      // 생성자 본문에서 플래그 설정
    bUpdateModule = false;
    ModuleName = "Spawn";
}
```

```cpp
// ❌ BAD - UE5 패턴 (Mundi에서 컴파일 안 됨)
#include "ParticleModule.h"                    // 상대 경로 없음
#include "ParticleModuleSpawn.generated.h"     // U prefix 없음

// Spawn 메서드: 3개 파라미터 (틀림!)
virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime) override;

UPROPERTY(EditAnywhere, Category="Spawn")
FVector LocationMin = FVector::ZeroVector;     // 헤더 초기화 금지, ZeroVector 없음
```

### 주요 API 차이점

| 항목 | UE5 | Mundi |
|------|-----|-------|
| FVector 제로 | `FVector::ZeroVector` | `FVector::Zero()` |
| FVector 1 | `FVector::OneVector` | `FVector::One()` |
| FVector 생성자 | `FVector(0,0,0)` | `FVector()` |
| FLinearColor 흰색 | `FLinearColor::White` | `FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)` |
| Generated 파일 | `ClassName.generated.h` | `UClassName.generated.h` |
| Spawn 파라미터 | 3개 | 4개 (ParticleBase 포함) |

---

## 📝 Phase 2.1: 모듈 헤더 파일 작성 (6개)

### 작업 순서

1. ParticleModuleSpawn.h
2. ParticleModuleLifetime.h
3. ParticleModuleLocation.h
4. ParticleModuleVelocity.h
5. ParticleModuleColor.h
6. ParticleModuleSize.h

---

### 1️⃣ ParticleModuleSpawn.h

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleSpawn.h`

**역할**: 파티클 생성 위치 제어 (Spawn 전용)

**체크리스트**:
- [ ] Include: `"../ParticleModule.h"`, `"UParticleModuleSpawn.generated.h"`
- [ ] DisplayName: `"생성 모듈"`
- [ ] UPROPERTY: `LocationMin`, `LocationMax`, `DistributionType`
- [ ] Constructor: 초기화 리스트에서 `FVector()` 사용
- [ ] Spawn 메서드: 4개 파라미터
- [ ] 플래그: `bSpawnModule = true`, 나머지 false

**코드 구조**:
```cpp
#pragma once

#include "../ParticleModule.h"
#include "UParticleModuleSpawn.generated.h"

UCLASS(DisplayName="생성 모듈", Description="파티클 생성 위치를 제어합니다")
class UParticleModuleSpawn : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleSpawn();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

    // 위치 분포
    UPROPERTY(EditAnywhere, Category="Location")
    FVector LocationMin;

    UPROPERTY(EditAnywhere, Category="Location")
    FVector LocationMax;

    UPROPERTY(EditAnywhere, Category="Location")
    EDistributionType DistributionType;
};
```

---

### 2️⃣ ParticleModuleLifetime.h

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleLifetime.h`

**역할**: 파티클 수명 제어 (Spawn 전용)

**체크리스트**:
- [ ] Include: `"../ParticleModule.h"`, `"UParticleModuleLifetime.generated.h"`
- [ ] DisplayName: `"수명 모듈"`
- [ ] UPROPERTY: `LifetimeMin`, `LifetimeMax`
- [ ] Constructor: 초기화 리스트에서 기본값 1.0f
- [ ] Spawn 메서드: 4개 파라미터
- [ ] 플래그: `bSpawnModule = true`

**코드 구조**:
```cpp
#pragma once

#include "../ParticleModule.h"
#include "UParticleModuleLifetime.generated.h"

UCLASS(DisplayName="수명 모듈", Description="파티클 수명을 제어합니다")
class UParticleModuleLifetime : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleLifetime();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

    // 수명 범위 (초 단위)
    UPROPERTY(EditAnywhere, Category="Lifetime")
    float LifetimeMin;

    UPROPERTY(EditAnywhere, Category="Lifetime")
    float LifetimeMax;
};
```

---

### 3️⃣ ParticleModuleLocation.h

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleLocation.h`

**역할**: 초기 위치 오프셋 (Spawn 전용)

**체크리스트**:
- [ ] Include: `"../ParticleModule.h"`, `"UParticleModuleLocation.generated.h"`
- [ ] DisplayName: `"위치 모듈"`
- [ ] UPROPERTY: `StartLocation` (FVector)
- [ ] Constructor: 초기화 리스트에서 `FVector()` 사용
- [ ] Spawn 메서드: 4개 파라미터
- [ ] 플래그: `bSpawnModule = true`

**코드 구조**:
```cpp
#pragma once

#include "../ParticleModule.h"
#include "UParticleModuleLocation.generated.h"

UCLASS(DisplayName="위치 모듈", Description="파티클 초기 위치를 설정합니다")
class UParticleModuleLocation : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleLocation();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

    // 시작 위치 오프셋
    UPROPERTY(EditAnywhere, Category="Location")
    FVector StartLocation;
};
```

---

### 4️⃣ ParticleModuleVelocity.h

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleVelocity.h`

**역할**: 초기 속도 설정 (Spawn 전용)

**체크리스트**:
- [ ] Include: `"../ParticleModule.h"`, `"UParticleModuleVelocity.generated.h"`
- [ ] DisplayName: `"속도 모듈"`
- [ ] UPROPERTY: `StartVelocityMin`, `StartVelocityMax`
- [ ] Constructor: 초기화 리스트에서 `FVector()` 사용
- [ ] Spawn 메서드: 4개 파라미터
- [ ] 플래그: `bSpawnModule = true`

**코드 구조**:
```cpp
#pragma once

#include "../ParticleModule.h"
#include "UParticleModuleVelocity.generated.h"

UCLASS(DisplayName="속도 모듈", Description="파티클 초기 속도를 설정합니다")
class UParticleModuleVelocity : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleVelocity();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

    // 초기 속도 범위
    UPROPERTY(EditAnywhere, Category="Velocity")
    FVector StartVelocityMin;

    UPROPERTY(EditAnywhere, Category="Velocity")
    FVector StartVelocityMax;
};
```

---

### 5️⃣ ParticleModuleColor.h

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleColor.h`

**역할**: 색상 제어 (Spawn + Update)

**체크리스트**:
- [ ] Include: `"../ParticleModule.h"`, `"UParticleModuleColor.generated.h"`
- [ ] DisplayName: `"색상 모듈"`
- [ ] UPROPERTY: `StartColor`, `EndColor`
- [ ] Constructor: 초기화 리스트에서 `FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)` 사용
- [ ] Spawn 메서드: 4개 파라미터
- [ ] Update 메서드: 3개 파라미터
- [ ] 플래그: `bSpawnModule = true`, `bUpdateModule = true`

**코드 구조**:
```cpp
#pragma once

#include "../ParticleModule.h"
#include "UParticleModuleColor.generated.h"

UCLASS(DisplayName="색상 모듈", Description="파티클 색상을 제어합니다")
class UParticleModuleColor : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleColor();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

    // 시작/종료 색상
    UPROPERTY(EditAnywhere, Category="Color")
    FLinearColor StartColor;

    UPROPERTY(EditAnywhere, Category="Color")
    FLinearColor EndColor;
};
```

---

### 6️⃣ ParticleModuleSize.h

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleSize.h`

**역할**: 크기 제어 (Spawn + Update)

**체크리스트**:
- [ ] Include: `"../ParticleModule.h"`, `"UParticleModuleSize.generated.h"`
- [ ] DisplayName: `"크기 모듈"`
- [ ] UPROPERTY: `StartSizeMin`, `StartSizeMax`
- [ ] Constructor: 초기화 리스트에서 `FVector()` 사용
- [ ] Spawn 메서드: 4개 파라미터
- [ ] Update 메서드: 3개 파라미터 (선택적)
- [ ] 플래그: `bSpawnModule = true`, `bUpdateModule = true`

**코드 구조**:
```cpp
#pragma once

#include "../ParticleModule.h"
#include "UParticleModuleSize.generated.h"

UCLASS(DisplayName="크기 모듈", Description="파티클 크기를 제어합니다")
class UParticleModuleSize : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleSize();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

    // 시작 크기 범위
    UPROPERTY(EditAnywhere, Category="Size")
    FVector StartSizeMin;

    UPROPERTY(EditAnywhere, Category="Size")
    FVector StartSizeMax;
};
```

---

## 🔨 Phase 2.2: GenerateBindings.bat 실행

**작업**: 6개 헤더 파일 작성 후 반드시 실행

```bash
cd c:\Users\Jungle\source\repos\KraftonGTL_week12\Mundi
GenerateBindings.bat
```

**생성 파일 확인**:
- `UParticleModuleSpawn.generated.h` / `.generated.cpp`
- `UParticleModuleLifetime.generated.h` / `.generated.cpp`
- `UParticleModuleLocation.generated.h` / `.generated.cpp`
- `UParticleModuleVelocity.generated.h` / `.generated.cpp`
- `UParticleModuleColor.generated.h` / `.generated.cpp`
- `UParticleModuleSize.generated.h` / `.generated.cpp`

---

## 💻 Phase 2.3: CPP 파일 구현 (2개)

### 1️⃣ ParticleModuleSpawn.cpp

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleSpawn.cpp`

**구현 가이드**:

```cpp
#include "pch.h"
#include "ParticleModuleSpawn.h"
#include "../ParticleTypes.h"
#include "../ParticleHelper.h"
#include <random>

UParticleModuleSpawn::UParticleModuleSpawn()
    : LocationMin(FVector())
    , LocationMax(FVector())
    , DistributionType(EDistributionType::Constant)
{
    bSpawnModule = true;
    bUpdateModule = false;
    bFinalUpdateModule = false;
    ModuleName = "Spawn";
}

void UParticleModuleSpawn::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    // MUNDI_SPAWN_INIT: 3개 파라미터 (Owner, Offset, ParticleBase)
    MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

    // 분포 타입별 위치 계산
    FVector SpawnLocation;

    switch (DistributionType)
    {
    case EDistributionType::Constant:
        SpawnLocation = LocationMin;
        break;

    case EDistributionType::Uniform:
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            float RandX = dist(gen);
            float RandY = dist(gen);
            float RandZ = dist(gen);

            SpawnLocation.X = LocationMin.X + (LocationMax.X - LocationMin.X) * RandX;
            SpawnLocation.Y = LocationMin.Y + (LocationMax.Y - LocationMin.Y) * RandY;
            SpawnLocation.Z = LocationMin.Z + (LocationMax.Z - LocationMin.Z) * RandZ;
        }
        break;

    default:
        SpawnLocation = LocationMin;
        break;
    }

    // 파티클 위치 설정
    Particle->Location = SpawnLocation;
    Particle->OldLocation = SpawnLocation;
}
```

**체크리스트**:
- [ ] pch.h include
- [ ] MUNDI_SPAWN_INIT 매크로 사용 (3개 파라미터)
- [ ] DistributionType에 따른 분기 처리
- [ ] std::random 사용 (Uniform 분포)
- [ ] Location과 OldLocation 모두 설정

---

### 2️⃣ ParticleModuleLifetime.cpp

**파일 경로**: `Mundi/Source/Runtime/Engine/Particles/Modules/ParticleModuleLifetime.cpp`

**구현 가이드**:

```cpp
#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "../ParticleTypes.h"
#include "../ParticleHelper.h"
#include <random>

UParticleModuleLifetime::UParticleModuleLifetime()
    : LifetimeMin(1.0f)
    , LifetimeMax(1.0f)
{
    bSpawnModule = true;
    bUpdateModule = false;
    bFinalUpdateModule = false;
    ModuleName = "Lifetime";
}

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

    // Min/Max 사이 랜덤 수명 계산
    float Lifetime = LifetimeMin;

    if (LifetimeMax > LifetimeMin)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        float RandValue = dist(gen);
        Lifetime = LifetimeMin + (LifetimeMax - LifetimeMin) * RandValue;
    }

    // OneOverMaxLifetime 최적화 적용
    if (Lifetime > 0.0f)
    {
        Particle->OneOverMaxLifetime = 1.0f / Lifetime;
    }
    else
    {
        Particle->OneOverMaxLifetime = 0.0f;
    }

    // RelativeTime 초기화
    Particle->RelativeTime = 0.0f;
}
```

**체크리스트**:
- [ ] pch.h include
- [ ] MUNDI_SPAWN_INIT 매크로 사용
- [ ] Min/Max 사이 랜덤 계산
- [ ] OneOverMaxLifetime 최적화 (1.0f / Lifetime)
- [ ] RelativeTime 초기화 (0.0f)
- [ ] Lifetime <= 0 예외 처리

---

## ✅ Phase 2.4: 컴파일 및 검증

### 컴파일 체크리스트

```bash
cd c:\Users\Jungle\source\repos\KraftonGTL_week12\Mundi
msbuild Mundi.sln /t:Build /p:Configuration=Debug
```

**예상 에러 및 해결**:

| 에러 유형 | 해결 방법 |
|-----------|----------|
| `cannot open include file 'UParticleModuleXXX.generated.h'` | GenerateBindings.bat 실행 확인 |
| `'ZeroVector': is not a member of 'FVector'` | `FVector::Zero()` 사용 |
| `'White': is not a member of 'FLinearColor'` | `FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)` 사용 |
| `no matching function for call to 'Spawn'` | 파라미터 4개 확인 (ParticleBase 포함) |

### 간단한 테스트 (Phase 2.5)

**목표**: ParticleModuleSpawn과 ParticleModuleLifetime이 정상 작동하는지 확인

**테스트 코드 예시** (나중에 작성):
```cpp
// 테스트: 100개 파티클 생성 시 위치 랜덤 분포 확인
// 테스트: 수명이 Min/Max 범위 내에 있는지 확인
```

---

## 📊 완료 기준

### Day 2 완료 조건

- [x] 6개 모듈 헤더 파일 작성
- [x] GenerateBindings.bat 실행 성공
- [x] 2개 CPP 파일 구현 (Spawn, Lifetime)
- [x] 컴파일 에러 0개
- [ ] 기본 파티클 생성 테스트 통과 (선택)

### 다음 단계 (Day 3)

1. 나머지 4개 모듈 CPP 구현 (Location, Velocity, Color, Size)
2. ParticleSystemComponent.cpp 구현 (UpdateParticles 로직)
3. 통합 테스트 및 디버깅

---

## 🚨 주의사항

### 절대 하지 말아야 할 것

1. ❌ UE5 코드 직접 복사-붙여넣기
2. ❌ 헤더에서 UPROPERTY 기본값 초기화 (`= FVector::ZeroVector`)
3. ❌ Spawn 메서드를 3개 파라미터로 작성
4. ❌ Generated 파일 이름에 U prefix 누락
5. ❌ FVector::ZeroVector, FLinearColor::White 같은 UE5 API 사용

### 반드시 해야 할 것

1. ✅ 생성자 초기화 리스트에서 모든 멤버 변수 초기화
2. ✅ Spawn 메서드 4개 파라미터 (ParticleBase 포함)
3. ✅ MUNDI_SPAWN_INIT 매크로 3개 파라미터로 호출
4. ✅ bSpawnModule/bUpdateModule 플래그 생성자에서 설정
5. ✅ 각 단계마다 컴파일 확인

---

## 📖 참고 파일

- [ParticleModule.h](../Source/Runtime/Engine/Particles/ParticleModule.h) - 기본 모듈 인터페이스
- [ParticleModuleRequired.h](../Source/Runtime/Engine/Particles/Modules/ParticleModuleRequired.h) - Required 모듈 참고
- [ParticleHelper.h](../Source/Runtime/Engine/Particles/ParticleHelper.h) - 헬퍼 매크로
- [ParticleTypes.h](../Source/Runtime/Engine/Particles/ParticleTypes.h) - 데이터 구조 및 Enum
- [CLAUDE.md](../CLAUDE.md) - Mundi 코드베이스 규칙

---

**작성일**: 2025-11-21
**작성자**: Claude Code
**버전**: 1.0
