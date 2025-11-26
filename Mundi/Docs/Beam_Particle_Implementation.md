# Beam 파티클 구현 원리

## 개요

Beam 파티클은 **시작점과 끝점을 연결하는 빔(광선)**을 표현하는 파티클 타입입니다. 레이저, 번개, 전기 충격, 추적 미사일 등에 사용됩니다.

핵심 원리:
- **Source-to-Target 구조**: 시작점에서 끝점으로 향하는 직선 빔
- **Segment 분할**: 빔을 여러 구간으로 나누어 곡선/노이즈 표현 가능
- **캐시 기반 노이즈**: 매 프레임 랜덤이 아닌 캐시된 노이즈로 부드러운 진동
- **Billboard 렌더링**: 항상 카메라를 향하는 Quad Strip

---

## 1. Beam 구조

### 1.1 Distance 모드 (현재 유일한 구현)

```
StartPos ━━━━━━━━━━━━━━━━━> EndPos
   ↑                            ↑
Particle.Location     StartPos + Direction * Length
```

**계산:**
```cpp
FVector StartPos = Particle->Location;
FVector BeamDir = TypeDataBeam->Direction;  // 정규화된 방향 벡터
FVector EndPos = StartPos + BeamDir * Length;
```

**특징:**
- 파티클 위치가 빔의 시작점
- Direction 벡터와 Length로 끝점 결정
- 각 파티클마다 독립적인 빔 생성

### 1.2 미래 확장 가능성

```cpp
enum class EBeamMethod : uint8
{
    Emitter,   // 에미터 위치 → 특정 위치
    Target,    // Actor A → Actor B
    Distance   // 현재 구현 (Particle → Direction * Length)
};
```

---

## 2. Segment 분할

### 2.1 왜 Segment가 필요한가?

**Segments = 1 (직선):**
```
Start ━━━━━━━━ End
```
- 노이즈 적용 불가
- 완벽한 직선

**Segments = 4:**
```
Start ━━ P1 ━━ P2 ━━ P3 ━━ End
         ↑     ↑     ↑
      내부 포인트 (노이즈 적용 가능)
```
- 각 세그먼트에 노이즈 적용 → 번개/전기 효과
- 부드러운 곡선 표현 가능

### 2.2 Segment 포인트 계산

```cpp
for (int32 s = 0; s <= Segments; ++s)
{
    // 선형 보간으로 각 세그먼트 위치 계산
    float T = (float)s / (float)Segments;
    // T: 0.0 (Start) → 0.25 → 0.5 → 0.75 → 1.0 (End)

    FVector SegPos = FMath::Lerp(StartPos, EndPos, T);
    // SegPos: 빔을 따라 균등 분포
}
```

**예시 (Segments=4):**
```
s=0: T=0.0  → StartPos
s=1: T=0.25 → StartPos + (EndPos-StartPos)*0.25
s=2: T=0.5  → 중간점
s=3: T=0.75 → StartPos + (EndPos-StartPos)*0.75
s=4: T=1.0  → EndPos
```

---

## 3. BeamNoise 모듈

### 3.1 노이즈 시스템 아키텍처

**문제:** 매 프레임 랜덤 생성 → 깜빡임 (지터링)

**해결:** 캐시 기반 노이즈
```cpp
struct FBeamNoiseCache
{
    TArray<TArray<FVector>> NoisePoints;      // [파티클][세그먼트]
    TArray<TArray<FVector>> NextNoisePoints;  // [파티클][세그먼트] (bSmooth용)
    TArray<float> NoiseTimers;                // [파티클]
};
```

**캐시 구조:**
```
Particle 0: [Noise0, Noise1, Noise2, ...]  (세그먼트당 노이즈 벡터)
Particle 1: [Noise0, Noise1, Noise2, ...]
Particle 2: [Noise0, Noise1, Noise2, ...]
```

### 3.2 NoiseLockTime 메커니즘

**NoiseLockTime = 0.1초:**
```
Timeline:
0.0s ━━━━━━━ 0.1s ━━━━━━━ 0.2s ━━━━━━━ 0.3s
  ↑ Generate   ↑ Generate   ↑ Generate
  Noise A      Noise B      Noise C

0.0~0.1s: Noise A 고정
0.1~0.2s: Noise B 고정
0.2~0.3s: Noise C 고정
```

**구현:**
```cpp
void Update(...)
{
    for (int32 p = 0; p < ParticleCount; ++p)
    {
        NoiseTimers[p] += DeltaTime;

        // NoiseLockTime 초과 시 노이즈 갱신
        if (NoiseTimers[p] >= NoiseLockTime)
        {
            NoiseTimers[p] = 0.0f;  // 타이머 리셋
            GenerateNoisePoints(NoisePoints[p], Segments);
        }
    }
}
```

### 3.3 bSmooth 보간

**bSmooth = false (순간 변경):**
```
NoiseLockTime 도달:
[Noise A] ━━━┓
              ┗━━━ [Noise B]
              ↑ 갑작스러운 변화
```

**bSmooth = true (부드러운 전환):**
```
NoiseLockTime 구간에서 보간:
[Noise A] ━━━━━━━━╲
                    ╲_____ [Noise B]
                    ↑ Lerp 적용
```

**구현:**
```cpp
if (bSmooth)
{
    // 현재 → 다음으로 복사
    NoisePoints[p] = NextNoisePoints[p];

    // 새 다음 노이즈 생성
    GenerateNoisePoints(NextNoisePoints[p], Segments);
}

// 렌더링 시 보간
float LerpAlpha = NoiseTimers[p] / NoiseLockTime;
FVector NoiseOffset = FMath::Lerp(
    NoisePoints[p][s],      // 현재
    NextNoisePoints[p][s],  // 다음
    LerpAlpha               // 0.0 → 1.0
);
```

**타임라인:**
```
Timer: 0.0 → 0.05 → 0.1 (NoiseLockTime)
Alpha: 0.0 → 0.5  → 1.0

0.0s:  100% Noise A + 0% Noise B
0.05s: 50% Noise A + 50% Noise B
0.1s:  0% Noise A + 100% Noise B
```

### 3.4 노이즈 생성

```cpp
void GenerateNoisePoints(TArray<FVector>& OutPoints, int32 SegmentCount)
{
    // 내부 포인트만 (시작/끝 제외)
    int32 NoisePointCount = FMath::Max(0, SegmentCount - 1);
    OutPoints.SetNum(NoisePointCount);

    // 랜덤 노이즈 생성
    for (int32 i = 0; i < NoisePointCount; ++i)
    {
        auto RandF = []() {
            return (float)rand() / RAND_MAX * 2.0f - 1.0f;  // -1.0 ~ 1.0
        };

        OutPoints[i] = FVector(
            RandF() * NoiseRange.X,  // Right 방향
            RandF() * NoiseRange.Y,  // Up 방향
            RandF() * NoiseRange.Z   // (일반적으로 0)
        );
    }
}
```

**NoiseRange 예시:**
```
NoiseRange = (0.2, 0.2, 0.0)

생성되는 노이즈:
(-0.15, +0.08, 0.0)  // 세그먼트 1
(+0.19, -0.12, 0.0)  // 세그먼트 2
(-0.03, +0.18, 0.0)  // 세그먼트 3
```

---

## 4. 렌더링 파이프라인

### 4.1 Billboard 좌표계 생성

빔은 항상 카메라를 향해야 합니다:

```cpp
// 1. 카메라 방향 계산
FVector ToCamera = (CameraPos - SegPos).GetSafeNormal();

// 2. Right 벡터: ToCamera와 BeamDir의 외적
FVector Right = FVector::Cross(ToCamera, BeamDir).GetSafeNormal();
```

**시각화:**
```
        Camera
          ↑ ToCamera
          |
          |
    ━━━━━●━━━━━  Beam (SegPos)
         Right →

Right = ToCamera × BeamDir (외적)
```

**예외 처리 (카메라가 빔과 평행):**
```cpp
if (Right.SizeSquared() < 0.001f)
{
    // Fallback: World Z축 사용
    Right = FVector::Cross(FVector(0,0,1), BeamDir).GetSafeNormal();
}
```

### 4.2 노이즈 적용

```cpp
// 노이즈는 시작/끝점 제외
if (bHasNoise && s > 0 && s < Segments)
{
    int32 NoiseIdx = s - 1;  // 내부 포인트 인덱스

    FVector NoiseOffset;
    if (bSmooth)
    {
        // 보간된 노이즈
        float LerpAlpha = NoiseTimers[p] / NoiseLockTime;
        NoiseOffset = FMath::Lerp(
            NoisePoints[p][NoiseIdx],
            NextNoisePoints[p][NoiseIdx],
            LerpAlpha
        );
    }
    else
    {
        NoiseOffset = NoisePoints[p][NoiseIdx];
    }

    // Right/Up 방향으로 오프셋 적용
    SegPos += Right * NoiseOffset.X + Up * NoiseOffset.Y;
}
```

**효과:**
```
노이즈 없음:
Start ━━━━━━━━ End

노이즈 적용:
Start ━╱━╲━╱━ End
      번개 효과
```

### 4.3 Taper (테이퍼링)

빔이 끝으로 갈수록 가늘어지는 효과:

```cpp
// T: 0.0 (Start) → 1.0 (End)
float TaperScale = 1.0f - (TaperFactor * T);
float HalfWidth = Width * 0.5f * Particle->Size.X * TaperScale;
```

**예시:**
```
TaperFactor = 0.0 (균일):
━━━━━━━━━━━━━━━━  Width 일정

TaperFactor = 0.5:
━━━━━━━━━━━━━━    시작: Width * 1.0
    ━━━━━━━━━━    중간: Width * 0.75
        ━━━━━━    끝:   Width * 0.5

TaperFactor = 1.0 (완전 테이퍼):
━━━━━━━━━━━━━━    시작: Width * 1.0
       ━━━━━━     중간: Width * 0.5
           ━      끝:   Width * 0.0
```

### 4.4 버텍스 생성

각 세그먼트마다 좌우 2개 버텍스 생성:

```cpp
for (int32 s = 0; s <= Segments; ++s)
{
    float T = (float)s / (float)Segments;
    FVector SegPos = Lerp(StartPos, EndPos, T);

    // Billboard Right 벡터 계산
    FVector Right = Cross(ToCamera, BeamDir).Normalized();

    // Taper 적용
    float TaperScale = 1.0f - (TaperFactor * T);
    float HalfWidth = Width * 0.5f * TaperScale;

    // 좌우 버텍스
    Vertices[BaseVertex + 0] = SegPos + Right * HalfWidth;  // 우측
    Vertices[BaseVertex + 1] = SegPos - Right * HalfWidth;  // 좌측

    // UV (T 기반)
    Vertices[BaseVertex + 0].UV = FVector2D(T, 1.0f);
    Vertices[BaseVertex + 1].UV = FVector2D(T, 0.0f);
}
```

**버텍스 레이아웃 (Segments=2):**
```
      V0  V2  V4
      ●───●───●  우측
Start ━━━━━━━━━ End
      ●───●───●  좌측
      V1  V3  V5

총 버텍스: (Segments+1) * 2 = 6개
```

### 4.5 인덱스 생성 (Quad Strip)

```cpp
for (int32 s = 0; s < Segments; ++s)
{
    int32 V0 = BaseVertex + s * 2;
    int32 V1 = V0 + 1;
    int32 V2 = V0 + 2;
    int32 V3 = V0 + 3;

    // Triangle 1: [V0, V2, V1]
    Indices[BaseIndex + 0] = V0;
    Indices[BaseIndex + 1] = V2;
    Indices[BaseIndex + 2] = V1;

    // Triangle 2: [V1, V2, V3]
    Indices[BaseIndex + 3] = V1;
    Indices[BaseIndex + 4] = V2;
    Indices[BaseIndex + 5] = V3;
}
```

**Quad 구성:**
```
V0 ━━━ V2
 ╲ ╲1  ╲
  ╲  ╲  ╲
   ╲2  ╲  ╲
    V1 ━━━ V3

Triangle 1: V0-V2-V1
Triangle 2: V1-V2-V3
```

---

## 5. Local/World Space

### 5.1 Local Space

파티클이 컴포넌트에 종속:

```cpp
if (bUseLocalSpace)
{
    // 위치 변환 (위치 + 회전 + 스케일)
    StartPos = CompTransform.TransformPosition(Particle->Location);

    // 방향 변환 (회전만, 스케일 X)
    BeamDir = CompTransform.TransformVector(BeamDirection).Normalized();
}
```

**효과:**
```
컴포넌트 회전 → 빔도 함께 회전
컴포넌트 이동 → 빔도 함께 이동
```

### 5.2 World Space

파티클이 월드에 고정:

```cpp
else
{
    StartPos = Particle->Location;     // 그대로
    BeamDir = BeamDirection;            // 그대로
}
```

**효과:**
```
컴포넌트 회전 → 빔은 고정 방향 유지
컴포넌트 이동 → 파티클은 월드 좌표계
```

---

## 6. 주요 프로퍼티

### TypeDataBeam 모듈

| 프로퍼티 | 타입 | 기본값 | 설명 |
|---------|------|--------|------|
| `BeamMethod` | enum | Distance | 빔 방식 (현재 Distance만) |
| `Segments` | int32 | 4 | 세그먼트 수 (1=직선) |
| `Width` | float | 0.05 | 빔 너비 (미터) |
| `Length` | float | 3.0 | 빔 길이 (미터) |
| `TaperFactor` | float | 0.0 | 테이퍼 정도 (0~1) |
| `Direction` | FVector | (1,0,0) | 빔 방향 (정규화) |

### BeamNoise 모듈

| 프로퍼티 | 타입 | 기본값 | 설명 |
|---------|------|--------|------|
| `NoiseRange` | FVector | (0.2,0.2,0) | 노이즈 범위 (미터) |
| `NoiseLockTime` | float | 0.1 | 노이즈 갱신 주기 (초) |
| `bSmooth` | bool | true | 부드러운 노이즈 보간 |

---

## 7. 성능 최적화

### 7.1 캐시 기반 노이즈

**Before (매 프레임 랜덤):**
```cpp
// 렌더링마다 생성 (60fps * N파티클 * M세그먼트)
for (Segment in Segments)
    Noise = Random();
```

**After (캐시 + 타이머):**
```cpp
// NoiseLockTime마다만 생성 (10fps * N파티클 * M세그먼트)
if (Timer >= NoiseLockTime)
    GenerateNoise();

// 렌더링은 캐시 읽기만
Noise = NoiseCache[Particle][Segment];
```

**성능 향상:**
- CPU: ~83% 감소 (60fps → 10fps 갱신)
- 메모리: 캐시 크기 = `ParticleCount * (Segments-1) * sizeof(FVector)` (무시 가능)

### 7.2 bSmooth의 트레이드오프

**bSmooth = false:**
- 메모리: `NoisePoints` 배열만 (50% 절약)
- 품질: 깜빡임 발생

**bSmooth = true:**
- 메모리: `NoisePoints + NextNoisePoints` (2배)
- 품질: 부드러운 전환
- CPU: Lerp 연산 추가 (미미함)

**권장:** bSmooth=true (메모리 비용 대비 품질 향상 큼)

---

## 8. 사용 예시

### 레이저 (직선)

```cpp
// TypeDataBeam
Segments = 1;           // 완벽한 직선
Width = 0.02f;          // 얇은 빔 (2cm)
Length = 100.0f;        // 긴 사거리
TaperFactor = 0.0f;     // 균일한 두께

// BeamNoise
NoiseRange = (0, 0, 0); // 노이즈 없음
```

### 번개 (지그재그)

```cpp
// TypeDataBeam
Segments = 8;           // 세밀한 굴곡
Width = 0.1f;           // 중간 두께 (10cm)
Length = 5.0f;          // 짧은 사거리
TaperFactor = 0.3f;     // 약간 테이퍼

// BeamNoise
NoiseRange = (0.5, 0.5, 0); // 큰 노이즈 (번개 효과)
NoiseLockTime = 0.05f;      // 빠른 깜빡임
bSmooth = false;            // 갑작스러운 변화 (번개 특성)
```

### 에너지 빔 (부드러운 진동)

```cpp
// TypeDataBeam
Segments = 6;
Width = 0.15f;          // 두꺼운 빔
Length = 10.0f;
TaperFactor = 0.5f;     // 끝으로 가늘어짐

// BeamNoise
NoiseRange = (0.1, 0.1, 0); // 적당한 노이즈
NoiseLockTime = 0.2f;       // 느린 갱신
bSmooth = true;             // 부드러운 전환
```

---

## 9. 고급 활용

### 9.1 다중 빔 (파티클 수 활용)

```cpp
// SpawnRate = 10/sec
// 파티클 10개 → 동시에 10개의 빔 렌더링

ParticleSystemComponent->SetSpawnRate(10.0f);
```

**효과:**
```
Beam 1 ━━━━━━━→
Beam 2 ━━━━━━━→
Beam 3 ━━━━━━━→
...
```

### 9.2 Color 모듈과 조합

```cpp
// Color Over Life
StartColor = FLinearColor(1.0, 1.0, 1.0, 1.0);  // 밝은 흰색
EndColor = FLinearColor(0.2, 0.2, 1.0, 0.0);    // 어두운 파란색으로 페이드
```

**효과:**
```
시작: 밝은 빔
━━━━━━━━━━━━━━

끝: 페이드아웃
━━━━━━━━ ─ ─ ─
```

### 9.3 Velocity와 조합 (추적 레이저)

```cpp
// Velocity 모듈로 파티클을 회전시킴
// → 빔 방향도 회전 (Local Space 시)

VelocityModule->InitialVelocity = Random Sphere;
```

---

## 10. 디버깅

### 10.1 노이즈 시각화

```cpp
// ParticleModuleBeamNoise.cpp에 추가
if (bDebugNoise)
{
    for (int32 p = 0; p < NoiseCache.NoisePoints.Num(); ++p)
    {
        for (FVector Noise : NoiseCache.NoisePoints[p])
        {
            DrawDebugSphere(Noise, 0.05f, Red);
        }
    }
}
```

### 10.2 Segment 포인트 시각화

```cpp
if (bDebugSegments)
{
    for (int32 s = 0; s <= Segments; ++s)
    {
        float T = (float)s / Segments;
        FVector SegPos = Lerp(StartPos, EndPos, T);
        DrawDebugSphere(SegPos, 0.1f, Green);
    }
}
```

---

## 요약

**Beam 파티클 핵심:**
1. **Segment 분할** - 직선 빔을 여러 구간으로 나누어 곡선/노이즈 표현
2. **캐시 기반 노이즈** - NoiseLockTime 간격으로 노이즈 갱신 (깜빡임 방지)
3. **bSmooth 보간** - 현재/다음 노이즈 간 Lerp로 부드러운 전환
4. **Billboard 렌더링** - 항상 카메라를 향하는 Quad Strip
5. **Taper** - 끝으로 갈수록 가늘어지는 효과

**성능 이점:**
- 노이즈 캐싱: ~83% 연산 감소 (매 프레임 → NoiseLockTime 간격)
- bSmooth 보간: 메모리 2배, 품질 대폭 향상
- Segment 기반: 유연한 품질/성능 조절

**사용 패턴:**
- **직선 레이저**: Segments=1, NoiseRange=0
- **번개**: Segments=8+, NoiseRange 큼, bSmooth=false
- **에너지 빔**: Segments=4~6, NoiseRange 적당, bSmooth=true
