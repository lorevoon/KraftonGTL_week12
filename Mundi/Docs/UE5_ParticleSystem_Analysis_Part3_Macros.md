# UE5 Particle System 분석 - Part 3: 헬퍼 매크로 및 구현 패턴

> 이전: [Part 2 - 핵심 클래스 분석](UE5_ParticleSystem_Analysis_Part2_Classes.md)

---

## 🔧 ParticleHelper.h 매크로 시스템

UE5는 파티클 모듈 구현의 **보일러플레이트 코드를 줄이기 위해** 다양한 매크로를 제공합니다.

**위치**: `Engine/Private/Particles/ParticleHelper.h`

---

## 📝 핵심 매크로 분석

### 1. DECLARE_PARTICLE_PTR

```cpp
// Engine/Private/Particles/ParticleHelper.h
#define DECLARE_PARTICLE_PTR(ParticleName, ParticleAddress) \
    FBaseParticle* ParticleName = (FBaseParticle*)(ParticleAddress)
```

**사용 예제**:
```cpp
virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
{
    BEGIN_UPDATE_LOOP
    {
        // uint8* 주소를 FBaseParticle*로 캐스팅
        DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);

        // Particle 사용
        Particle->Velocity += Acceleration * DeltaTime;
    }
    END_UPDATE_LOOP
}
```

**장점**:
- ✅ 타입 안전성 (명시적 캐스팅)
- ✅ 가독성 향상
- ✅ 일관된 코드 스타일

---

### 2. BEGIN_UPDATE_LOOP / END_UPDATE_LOOP

```cpp
// Engine/Private/Particles/ParticleHelper.h
#define BEGIN_UPDATE_LOOP \
    for (int32 i = 0; i < Owner->ActiveParticles; i++) \
    {

#define END_UPDATE_LOOP \
    }
```

**사용 예제**:
```cpp
virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
{
    FVector AccelValue = Acceleration.GetValue(Owner->EmitterTime);

    BEGIN_UPDATE_LOOP
    {
        DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);
        Particle->Velocity += AccelValue * DeltaTime;
    }
    END_UPDATE_LOOP
}
```

**매크로 확장 결과**:
```cpp
// 컴파일러가 보는 코드
for (int32 i = 0; i < Owner->ActiveParticles; i++)
{
    FBaseParticle* Particle = (FBaseParticle*)(Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);
    Particle->Velocity += AccelValue * DeltaTime;
}
```

**장점**:
- ✅ 활성 파티클만 순회 (인덱스 배열 활용)
- ✅ 일관된 루프 패턴
- ✅ 향후 병렬화/SIMD 최적화 적용 용이

---

### 3. SPAWN_INIT

```cpp
// Engine/Private/Particles/ParticleHelper.h
#define SPAWN_INIT \
    FMemory::Memzero(ParticleBase, sizeof(FBaseParticle));
```

**사용 예제**:
```cpp
virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
{
    // 파티클 메모리 초기화 (0으로)
    SPAWN_INIT;

    // 초기값 설정
    ParticleBase->Location = SpawnLocation.GetValue(SpawnTime);
    ParticleBase->Velocity = InitialVelocity.GetValue(SpawnTime);
}
```

**목적**:
- 새 파티클 메모리 영역을 0으로 초기화
- 이전 파티클의 잔여 데이터 제거
- 예측 가능한 초기 상태 보장

**주의**:
- `SPAWN_INIT` 후 필수 필드(`OneOverMaxLifetime` 등) 반드시 설정
- Required 모듈이 보통 첫 번째로 실행되어 초기화 담당

---

### 4. PARTICLE_ELEMENT

```cpp
// 모듈별 추가 데이터 접근 매크로
#define PARTICLE_ELEMENT(Type, Offset) \
    *((Type*)((uint8*)ParticleBase + Offset))
```

**사용 예제**:
```cpp
// MeshRotation 모듈의 추가 페이로드
struct FMeshRotationPayload
{
    FVector Rotation;
    FVector RotationRate;
};

virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
{
    // Offset 위치의 페이로드 접근
    FMeshRotationPayload& Payload = PARTICLE_ELEMENT(FMeshRotationPayload, Offset);

    Payload.Rotation = InitialRotation.GetValue(SpawnTime);
    Payload.RotationRate = RotationRate.GetValue(SpawnTime);
}
```

**메모리 레이아웃**:
```
[FBaseParticle: 128B] [MeshRotationPayload: 24B] [CollisionPayload: 16B]
^                      ^                          ^
ParticleBase          ParticleBase + Offset1    ParticleBase + Offset2
```

---

### 5. GET_PARTICLE_DIRECT

```cpp
// 인덱스 기반 직접 파티클 접근
#define GET_PARTICLE_DIRECT(Index) \
    (FBaseParticle*)(Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[Index])
```

**사용 예제**:
```cpp
// 특정 파티클에만 영향 (예: 첫 파티클만)
FBaseParticle* FirstParticle = GET_PARTICLE_DIRECT(0);
FirstParticle->Size *= 2.0f;  // 첫 파티클만 크기 2배
```

---

## 🎯 실전 모듈 구현 패턴

### Pattern 1: Spawn-Only 모듈

```cpp
// 예: ParticleModuleLocation
class UParticleModuleLocation : public UParticleModule
{
    UPROPERTY(EditAnywhere)
    FRawDistributionVector StartLocation;

    UParticleModuleLocation()
    {
        bSpawnModule = true;   // ✅
        bUpdateModule = false; // ❌
    }

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
    {
        // 초기 위치 설정
        FVector Location = StartLocation.GetValue(SpawnTime, Owner->Component);
        ParticleBase->Location = Location;
        ParticleBase->OldLocation = Location;  // 첫 프레임 델타 방지
    }
};
```

**특징**:
- Spawn 단계에서만 실행
- 초기값 설정만 수행
- 성능 효율적 (Update 루프 제외)

---

### Pattern 2: Update-Only 모듈

```cpp
// 예: ParticleModuleAcceleration
class UParticleModuleAcceleration : public UParticleModule
{
    UPROPERTY(EditAnywhere)
    FRawDistributionVector Acceleration;

    UParticleModuleAcceleration()
    {
        bSpawnModule = false; // ❌
        bUpdateModule = true; // ✅
    }

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
    {
        // 가속도 값 캐싱 (루프 밖)
        FVector AccelValue = Acceleration.GetValue(Owner->EmitterTime, Owner->Component);

        BEGIN_UPDATE_LOOP
        {
            DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);

            // 매 프레임 속도 누적
            Particle->Velocity += AccelValue * DeltaTime;
        }
        END_UPDATE_LOOP
    }
};
```

**최적화 포인트**:
- ✅ `GetValue()` 루프 밖에서 호출 (상수 가속도)
- ✅ `BEGIN_UPDATE_LOOP` 매크로로 간결한 코드
- ✅ DeltaTime 적용 (물리적으로 올바른 통합)

---

### Pattern 3: Spawn + Update 모듈

```cpp
// 예: ParticleModuleRotationRate
class UParticleModuleRotationRate : public UParticleModule
{
    UPROPERTY(EditAnywhere)
    FRawDistributionFloat StartRotationRate;

    UParticleModuleRotationRate()
    {
        bSpawnModule = true;  // ✅ Spawn에서 초기 회전속도 설정
        bUpdateModule = true; // ✅ Update에서 회전 적용
    }

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
    {
        // 초기 회전 속도 설정
        float RotRate = StartRotationRate.GetValue(SpawnTime, Owner->Component);
        ParticleBase->BaseRotationRate = RotRate;
    }

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
    {
        BEGIN_UPDATE_LOOP
        {
            DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);

            // 매 프레임 회전 적용
            Particle->Rotation += Particle->BaseRotationRate * DeltaTime;
        }
        END_UPDATE_LOOP
    }
};
```

**Dual State 활용**:
- `BaseRotationRate`: Spawn에서 설정, Update에서 읽기 전용
- 다른 모듈(RotationRateOverLife)이 `BaseRotationRate` 기반으로 계산 가능

---

### Pattern 4: 커브 기반 오버라이프 모듈

```cpp
// 예: ParticleModuleSizeOverLife
class UParticleModuleSizeOverLife : public UParticleModule
{
    UPROPERTY(EditAnywhere)
    FRawDistributionVector SizeOverLife;

    UParticleModuleSizeOverLife()
    {
        bSpawnModule = false;
        bUpdateModule = true;
    }

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
    {
        BEGIN_UPDATE_LOOP
        {
            DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);

            // RelativeTime (0.0 ~ 1.0) 기반 커브 샘플링
            float t = Particle->RelativeTime;
            FVector SizeScale = SizeOverLife.GetValue(t, Owner->Component);

            // BaseSize와 곱셈 (Multiplicative)
            Particle->Size.X = Particle->BaseSize.X * SizeScale.X;
            Particle->Size.Y = Particle->BaseSize.Y * SizeScale.Y;
            Particle->Size.Z = Particle->BaseSize.Z * SizeScale.Z;
        }
        END_UPDATE_LOOP
    }
};
```

**핵심 패턴**:
- ✅ `RelativeTime` 활용 → 생명 주기 독립적
- ✅ `BaseSize` 곱셈 → 원본 크기 유지
- ✅ 매 프레임 재계산 → 실시간 커브 변경 반영

---

## 🚀 성능 최적화 기법

### 1. 루프 밖 계산

```cpp
// ❌ Bad: 루프 안에서 매번 계산
virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
{
    BEGIN_UPDATE_LOOP
    {
        DECLARE_PARTICLE_PTR(Particle, ...);

        // 모든 파티클에 동일한 값인데 매번 계산!
        FVector Gravity = GravityScale.GetValue(Owner->EmitterTime);
        Particle->Velocity += Gravity * DeltaTime;
    }
    END_UPDATE_LOOP
}

// ✅ Good: 루프 밖에서 한 번만 계산
virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
{
    FVector Gravity = GravityScale.GetValue(Owner->EmitterTime);  // 한 번만!

    BEGIN_UPDATE_LOOP
    {
        DECLARE_PARTICLE_PTR(Particle, ...);
        Particle->Velocity += Gravity * DeltaTime;
    }
    END_UPDATE_LOOP
}
```

---

### 2. Early Exit

```cpp
virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
{
    // 활성 파티클 없으면 즉시 리턴
    if (Owner->ActiveParticles <= 0)
        return;

    // 모듈이 비활성화 상태면 리턴
    if (!bEnabled)
        return;

    // 실제 업데이트 로직
    BEGIN_UPDATE_LOOP
    {
        // ...
    }
    END_UPDATE_LOOP
}
```

---

### 3. 조건부 실행 (LOD)

```cpp
virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
{
    // 높은 LOD에서만 실행 (디테일 감소)
    if (Owner->CurrentLODLevel->Level > MaxLODLevel)
        return;

    BEGIN_UPDATE_LOOP
    {
        // 고품질 계산...
    }
    END_UPDATE_LOOP
}
```

---

### 4. SIMD 준비 (미래 최적화)

```cpp
// UE5는 매크로로 추상화 → 향후 SIMD 변환 가능
BEGIN_UPDATE_LOOP
{
    // 현재: 스칼라 연산
    Particle->Velocity += Acceleration * DeltaTime;
}
END_UPDATE_LOOP

// 향후 매크로 재정의로 SIMD 적용 가능:
// #define BEGIN_UPDATE_LOOP \
//     for (int32 i = 0; i < Owner->ActiveParticles; i += 4) \
//     {   \
//         VectorRegister V = VectorLoadAligned(&Particles[i]);
```

---

## 📋 모듈 구현 체크리스트

### Spawn 단계 체크리스트
- [ ] `bSpawnModule = true` 설정
- [ ] `SPAWN_INIT` 필요 시 호출 (보통 Required 모듈이 처리)
- [ ] `Base*` 필드 초기화 (BaseSize, BaseColor 등)
- [ ] 현재 필드도 함께 초기화 (Size = BaseSize)
- [ ] `OldLocation = Location` 설정 (첫 프레임 델타 방지)

### Update 단계 체크리스트
- [ ] `bUpdateModule = true` 설정
- [ ] 상수 값은 루프 밖에서 계산
- [ ] `BEGIN_UPDATE_LOOP` / `END_UPDATE_LOOP` 사용
- [ ] `DECLARE_PARTICLE_PTR` 매크로로 파티클 접근
- [ ] `RelativeTime` 기반 커브 샘플링 (오버라이프 모듈)
- [ ] `Base*` 필드 기반 곱셈 (Multiplicative 모듈)

### Final Update 단계 체크리스트
- [ ] `bFinalUpdateModule = true` 설정
- [ ] 렌더링 직전 처리만 수행 (정렬, 빌보드 회전 등)
- [ ] 물리 상태 변경 금지 (Location/Velocity 수정 X)

---

## 🎯 Mundi Engine 적용 권장사항

### 1. 매크로 시스템 도입

```cpp
// Mundi/Source/Runtime/Engine/Public/Particles/ParticleHelper.h
#pragma once

// 파티클 포인터 선언
#define MUNDI_DECLARE_PARTICLE_PTR(ParticleName, ParticleAddress) \
    FMundiBaseParticle* ParticleName = (FMundiBaseParticle*)(ParticleAddress)

// Update 루프
#define MUNDI_BEGIN_UPDATE_LOOP \
    for (int32 i = 0; i < Owner->ActiveParticles; i++) \
    {

#define MUNDI_END_UPDATE_LOOP \
    }

// Spawn 초기화
#define MUNDI_SPAWN_INIT \
    FMemory::Memzero(ParticleBase, sizeof(FMundiBaseParticle));
```

**장점**:
- ✅ UE5와 동일한 코드 스타일
- ✅ 향후 최적화 적용 용이
- ✅ 보일러플레이트 코드 감소

---

### 2. Context Pattern 도입 (선택사항)

```cpp
// Person 1 작업 범위 외 (향후 리팩토링)
struct FMundiSpawnContext
{
    FMundiParticleEmitterInstance& Owner;
    int32 Offset;
    float SpawnTime;
    FMundiBaseParticle* ParticleBase;
};

// 기존 인터페이스 유지하고 오버로드 추가
virtual void Spawn(FMundiParticleEmitterInstance* Owner, int32 Offset, float SpawnTime);
virtual void Spawn(const FMundiSpawnContext& Context);  // 새 버전
```

---

## 🎯 다음 Part 예고

**Part 4**에서는 Mundi 구현 계획과의 비교 및 최종 권장사항:
- UE5 vs Mundi 아키텍처 차이점 정리
- 우선순위별 개선 권장사항
- Person 1 작업 범위 명확화
- 향후 리팩토링 로드맵

---

**작성일**: 2025-11-21
**분석 기준**: Unreal Engine 5.x Source Code
