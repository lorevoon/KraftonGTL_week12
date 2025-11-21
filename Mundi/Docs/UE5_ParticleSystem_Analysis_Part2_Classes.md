# UE5 Particle System 분석 - Part 2: 핵심 클래스 분석

> 이전: [Part 1 - 개요 및 아키텍처](UE5_ParticleSystem_Analysis_Part1_Overview.md)

---

## 🎨 UParticleModule 베이스 클래스

### 전체 인터페이스

```cpp
// Engine/Classes/Particles/ParticleModule.h
UCLASS(editinlinenew, hidecategories=Object, abstract)
class UParticleModule : public UObject
{
    GENERATED_UCLASS_BODY()

    // ===== 실행 단계 플래그 =====
    UPROPERTY()
    uint8 bSpawnModule:1;           // Spawn 단계에서 실행

    UPROPERTY()
    uint8 bUpdateModule:1;          // Update 단계에서 실행

    UPROPERTY()
    uint8 bFinalUpdateModule:1;     // FinalUpdate 단계에서 실행

    UPROPERTY()
    uint8 bCurvesAsColor:1;         // 커브를 컬러로 해석

    // ===== 가상 함수: Spawn =====
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
    {
        // 기본 구현: 아무것도 하지 않음
    }

    // ===== 가상 함수: Update =====
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
    {
        // 기본 구현: 아무것도 하지 않음
    }

    // ===== 가상 함수: Final Update =====
    virtual void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
    {
        // 기본 구현: 아무것도 하지 않음
    }

    // ===== Emitter 초기화 =====
    virtual uint32 RequiredBytes(UParticleLODLevel* LODLevel = nullptr)
    {
        // 이 모듈이 파티클당 추가로 필요한 바이트 수 반환
        // 예: MeshRotation 모듈 → sizeof(FMeshRotationPayload)
        return 0;
    }

    virtual void SetToSensibleDefaults(UParticleEmitter* Owner)
    {
        // 에디터에서 모듈 추가 시 기본값 설정
    }
};
```

### 핵심 특징

#### 1. **실행 단계 플래그 시스템**
```cpp
// LOD 레벨에서 모듈 리스트 캐싱
void UParticleLODLevel::UpdateModuleLists()
{
    SpawnModules.Empty();
    UpdateModules.Empty();
    FinalUpdateModules.Empty();

    for (UParticleModule* Module : Modules)
    {
        if (Module->bSpawnModule)
            SpawnModules.Add(Module);
        if (Module->bUpdateModule)
            UpdateModules.Add(Module);
        if (Module->bFinalUpdateModule)
            FinalUpdateModules.Add(Module);
    }
}
```

**장점**:
- ✅ 하나의 모듈이 여러 단계에서 실행 가능
- ✅ 런타임에 불필요한 모듈 스킵 (if 체크만으로)
- ✅ LOD별 모듈 리스트 캐싱으로 성능 최적화

**Mundi 계획과의 차이**:
- Mundi: `ExecutionOrder` 정수값 → 하나의 단계만 지정 가능
- UE5: 비트 플래그 → 유연한 다단계 실행

#### 2. **동적 페이로드 (RequiredBytes)**
```cpp
// 예: MeshRotation 모듈
struct FMeshRotationPayload
{
    FVector Rotation;
    FVector RotationRate;
};

class UParticleModuleMeshRotation : public UParticleModule
{
    virtual uint32 RequiredBytes(UParticleLODLevel* LODLevel) override
    {
        return sizeof(FMeshRotationPayload);
    }

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
    {
        // Offset 위치에서 추가 데이터 접근
        FMeshRotationPayload* Payload = (FMeshRotationPayload*)((uint8*)ParticleBase + Offset);
        Payload->Rotation = InitialRotation.GetValue(SpawnTime);
        Payload->RotationRate = RotationRate.GetValue(SpawnTime);
    }
};
```

**메모리 레이아웃**:
```
[FBaseParticle: 128 bytes] [MeshRotationPayload: 24 bytes] [OtherPayload: ?? bytes]
^                           ^
ParticleData               ParticleData + Offset
```

---

## 🔄 Context Pattern (UE5의 개선된 패턴)

### 실제 UE5 구현

```cpp
// Engine/Private/Particles/ParticleHelper.h

// Spawn Context
struct FSpawnContext
{
    FParticleEmitterInstance& Owner;
    int32 Offset;
    float SpawnTime;
    FBaseParticle* ParticleBase;

    // 헬퍼 함수
    FORCEINLINE FBaseParticle* GetParticle() const { return ParticleBase; }
    FORCEINLINE uint8* GetModuleInstanceData() const { return Owner.GetModuleInstanceData(Offset); }
};

// Update Context
struct FUpdateContext
{
    FParticleEmitterInstance& Owner;
    int32 Offset;
    float DeltaTime;

    FORCEINLINE FBaseParticle* GetParticle(int32 Index) const
    {
        return Owner.GetParticle(Index);
    }
};

// 개선된 모듈 인터페이스 (일부 신규 모듈)
class UParticleModuleColorOverLife : public UParticleModule
{
    virtual void Spawn(const FSpawnContext& Context) override
    {
        FBaseParticle* Particle = Context.GetParticle();
        Particle->BaseColor = ColorOverLife.GetValue(Context.SpawnTime);
    }

    virtual void Update(const FUpdateContext& Context) override
    {
        BEGIN_UPDATE_LOOP
        {
            FBaseParticle* Particle = Context.GetParticle(i);
            float t = Particle->RelativeTime;
            Particle->Color = Particle->BaseColor * ColorOverLife.GetValue(t);
        }
        END_UPDATE_LOOP
    }
};
```

### 장점

#### 1. **타입 안전성**
```cpp
// Bad: 실수로 잘못된 파라미터 전달
Module->Spawn(Owner, DeltaTime, Offset, ParticleBase);  // ❌ 순서 바뀜

// Good: 타입으로 구분
FSpawnContext Context = { Owner, Offset, SpawnTime, ParticleBase };
Module->Spawn(Context);  // ✅ 컴파일 타임 검증
```

#### 2. **확장성**
```cpp
// 새 필드 추가 시 기존 코드 영향 최소화
struct FSpawnContext
{
    // 기존 필드...
    FRandomStream* RandomStream;  // 새 필드 추가
    int32 ParticleIndex;          // 새 필드 추가

    // 기본값 제공으로 하위 호환성 유지
};
```

#### 3. **헬퍼 함수 제공**
```cpp
Context.GetParticle();               // 간결한 접근
Context.GetModuleInstanceData();     // 모듈별 인스턴스 데이터
Context.Owner.GetCurrentLODLevel();  // LOD 정보
```

---

## 🎯 실제 모듈 구현 예제

### 1. UParticleModuleLifetime

```cpp
// Engine/Classes/Particles/Lifetime/ParticleModuleLifetime.h
UCLASS(editinlinenew, hidecategories=Object, MinimalAPI)
class UParticleModuleLifetime : public UParticleModule
{
    GENERATED_UCLASS_BODY()

    // 생명 시간 분포 (커브 또는 랜덤 범위)
    UPROPERTY(EditAnywhere, Category=Lifetime)
    FRawDistributionFloat Lifetime;

    // 생성자
    UParticleModuleLifetime()
    {
        bSpawnModule = true;  // Spawn 단계에서만 실행
        bUpdateModule = false;
    }

    // Spawn 구현
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
    {
        // Lifetime 분포에서 값 샘플링
        float MaxLifetime = Lifetime.GetValue(SpawnTime, Owner->Component);

        if (MaxLifetime > 0.0f)
        {
            // FBaseParticle 초기화
            ParticleBase->OneOverMaxLifetime = 1.0f / MaxLifetime;
            ParticleBase->RelativeTime = 0.0f;
        }
        else
        {
            // 즉시 죽은 파티클로 표시
            ParticleBase->RelativeTime = 1.1f;
        }
    }
};
```

**핵심 포인트**:
- `OneOverMaxLifetime` 저장 → Update에서 나눗셈 연산 제거
- `RelativeTime = 0.0f` → Spawn 시 초기화
- `RelativeTime > 1.0f` → 파티클 사망 판정

---

### 2. UParticleModuleColor

```cpp
// Engine/Classes/Particles/Color/ParticleModuleColor.h
UCLASS(editinlinenew, hidecategories=Object, MinimalAPI)
class UParticleModuleColor : public UParticleModule
{
    GENERATED_UCLASS_BODY()

    // 초기 컬러 분포
    UPROPERTY(EditAnywhere, Category=Color)
    FRawDistributionVector StartColor;

    // 초기 알파 분포
    UPROPERTY(EditAnywhere, Category=Color)
    FRawDistributionFloat StartAlpha;

    UParticleModuleColor()
    {
        bSpawnModule = true;
        bUpdateModule = false;
    }

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override
    {
        // StartColor 샘플링 (RGB)
        FVector ColorVec = StartColor.GetValue(SpawnTime, Owner->Component);

        // StartAlpha 샘플링
        float Alpha = StartAlpha.GetValue(SpawnTime, Owner->Component);

        // Base와 Current 모두 설정
        ParticleBase->BaseColor = FLinearColor(ColorVec.X, ColorVec.Y, ColorVec.Z, Alpha);
        ParticleBase->Color = ParticleBase->BaseColor;
    }
};
```

**Dual State 활용**:
- `BaseColor` 저장 → `ColorOverLife` 모듈이 이를 기준으로 계산
- `Color = BaseColor` → 초기 렌더링 컬러

---

### 3. UParticleModuleColorOverLife

```cpp
// Engine/Classes/Particles/Color/ParticleModuleColorOverLife.h
UCLASS(editinlinenew, hidecategories=Object, MinimalAPI)
class UParticleModuleColorOverLife : public UParticleModule
{
    GENERATED_UCLASS_BODY()

    // 생명 주기에 따른 컬러 커브
    UPROPERTY(EditAnywhere, Category=Color)
    FRawDistributionVector ColorOverLife;

    // 생명 주기에 따른 알파 커브
    UPROPERTY(EditAnywhere, Category=Color)
    FRawDistributionFloat AlphaOverLife;

    UParticleModuleColorOverLife()
    {
        bSpawnModule = false;
        bUpdateModule = true;  // Update 단계에서 실행
    }

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
    {
        BEGIN_UPDATE_LOOP  // 매크로: for (int32 i = 0; i < ActiveParticles; i++)
        {
            // 현재 파티클 가져오기
            DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);

            // RelativeTime 기반 커브 샘플링
            float t = Particle->RelativeTime;
            FVector ColorVec = ColorOverLife.GetValue(t, Owner->Component);
            float Alpha = AlphaOverLife.GetValue(t, Owner->Component);

            // Base 컬러에 커브 곱셈 (Additive/Multiplicative)
            Particle->Color.R = Particle->BaseColor.R * ColorVec.X;
            Particle->Color.G = Particle->BaseColor.G * ColorVec.Y;
            Particle->Color.B = Particle->BaseColor.B * ColorVec.Z;
            Particle->Color.A = Particle->BaseColor.A * Alpha;
        }
        END_UPDATE_LOOP
    }
};
```

**BaseColor 활용**:
- ✅ `BaseColor` 유지 → 매 프레임 원본 기준 계산
- ✅ 여러 ColorOverLife 모듈 체이닝 가능
- ✅ Flickering 방지 (누적 오차 없음)

**Mundi 계획과의 차이**:
- Mundi: Base 필드 없음 → `Color *= CurveValue` 형태 (누적)
- UE5: Base 필드 있음 → `Color = BaseColor * CurveValue` (절대값)

---

### 4. UParticleModuleAcceleration

```cpp
// Engine/Classes/Particles/Acceleration/ParticleModuleAcceleration.h
UCLASS(editinlinenew, hidecategories=Object, MinimalAPI)
class UParticleModuleAcceleration : public UParticleModule
{
    GENERATED_UCLASS_BODY()

    // 가속도 벡터 분포
    UPROPERTY(EditAnywhere, Category=Acceleration)
    FRawDistributionVector Acceleration;

    // 매 프레임 적용 여부
    UPROPERTY(EditAnywhere, Category=Acceleration)
    uint8 bApplyOwnerScale:1;

    UParticleModuleAcceleration()
    {
        bSpawnModule = false;
        bUpdateModule = true;
    }

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override
    {
        // 가속도 값 가져오기
        FVector AccelValue = Acceleration.GetValue(Owner->EmitterTime, Owner->Component);

        if (bApplyOwnerScale)
        {
            AccelValue *= Owner->Component->GetComponentScale();
        }

        BEGIN_UPDATE_LOOP
        {
            DECLARE_PARTICLE_PTR(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);

            // 속도에 가속도 누적
            Particle->Velocity += AccelValue * DeltaTime;
        }
        END_UPDATE_LOOP
    }
};
```

**물리 통합**:
```cpp
// FParticleEmitterInstance::Tick()
// 1. Update 단계에서 모듈이 Velocity 수정
for (UParticleModule* Module : UpdateModules)
    Module->Update(this, Offset, DeltaTime);

// 2. Physics Integration
BEGIN_UPDATE_LOOP
{
    Particle->OldLocation = Particle->Location;
    Particle->Location += Particle->Velocity * DeltaTime;
    Particle->RelativeTime += Particle->OneOverMaxLifetime * DeltaTime;
}
END_UPDATE_LOOP
```

---

## 📊 FParticleEmitterInstance 전체 구조

```cpp
// Engine/Public/Particles/ParticleEmitterInstance.h
struct FParticleEmitterInstance
{
    // ===== Owner 정보 =====
    UParticleSystemComponent* Component;
    UParticleLODLevel* CurrentLODLevel;
    int32 ActiveParticles;

    // ===== 메모리 풀 =====
    uint8* ParticleData;              // 파티클 데이터 버퍼
    int32* ParticleIndices;           // 활성 파티클 인덱스
    uint32 ParticleStride;            // 파티클당 바이트 크기
    int32 MaxActiveParticles;         // 최대 파티클 수

    // ===== 시간 정보 =====
    float EmitterTime;                // 이미터 누적 시간
    float EmitterDuration;            // 이미터 총 지속 시간
    int32 LoopCount;                  // 반복 횟수

    // ===== LOD 캐싱 =====
    TArray<UParticleModule*> SpawnModules;       // bSpawnModule = true인 모듈만
    TArray<UParticleModule*> UpdateModules;      // bUpdateModule = true인 모듈만
    TArray<UParticleModule*> FinalUpdateModules; // bFinalUpdateModule = true인 모듈만

    // ===== 핵심 함수 =====
    void Tick(float DeltaTime, bool bSuppressSpawning);
    void SpawnParticles(int32 Count, float SpawnTime);
    void KillParticle(int32 Index);
    FBaseParticle* GetParticle(int32 Index);
};
```

### Tick 흐름도

```cpp
void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
    // 1. Spawn 단계
    if (!bSuppressSpawning)
    {
        int32 SpawnCount = CalculateSpawnCount(DeltaTime);
        for (int32 i = 0; i < SpawnCount; i++)
        {
            int32 ParticleIndex = AllocateParticle();
            FBaseParticle* Particle = GetParticle(ParticleIndex);

            // Spawn 모듈 실행
            for (UParticleModule* Module : SpawnModules)
            {
                Module->Spawn(this, ModuleOffset, SpawnTime, Particle);
                ModuleOffset += Module->RequiredBytes();
            }
        }
    }

    // 2. Update 단계
    for (UParticleModule* Module : UpdateModules)
    {
        Module->Update(this, ModuleOffset, DeltaTime);
        ModuleOffset += Module->RequiredBytes();
    }

    // 3. Physics Integration
    BEGIN_UPDATE_LOOP
    {
        Particle->OldLocation = Particle->Location;
        Particle->Location += Particle->Velocity * DeltaTime;
        Particle->RelativeTime += Particle->OneOverMaxLifetime * DeltaTime;

        // 사망 판정
        if (Particle->RelativeTime > 1.0f)
            KillParticle(i);
    }
    END_UPDATE_LOOP

    // 4. Final Update 단계
    for (UParticleModule* Module : FinalUpdateModules)
    {
        Module->FinalUpdate(this, ModuleOffset, DeltaTime);
        ModuleOffset += Module->RequiredBytes();
    }

    // 5. Rendering 데이터 준비
    UpdateBoundingBox();
    PrepareForRendering();
}
```

---

## 🎯 다음 Part 예고

**Part 3**에서는 헬퍼 매크로와 구현 패턴:
- `BEGIN_UPDATE_LOOP` / `END_UPDATE_LOOP` 매크로
- `DECLARE_PARTICLE_PTR` 매크로
- `SPAWN_INIT` 매크로
- 모듈 구현 Best Practices
- 성능 최적화 기법

---

**작성일**: 2025-11-21
**분석 기준**: Unreal Engine 5.x Source Code
