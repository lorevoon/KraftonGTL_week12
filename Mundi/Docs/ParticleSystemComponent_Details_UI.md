# 파티클 시스템 컴포넌트 디테일 패널 UI 구현 가이드

## 목표
- 액터에 `UParticleSystemComponent`가 붙어 있을 때 디테일 패널에서 아래 컨트롤을 노출한다.
  - **Template 드롭다운**: 프로젝트에 존재하는 `UParticleSystem` 에셋 중 하나를 선택.
  - **Reset 버튼**: 인스턴스 상태(파티클 풀, 루프 카운트 등)를 초기화.
  - **Custom Playback Rate**: `Tick`에 전달되는 `DeltaTime`에 곱해지는 배율 입력.
- 기존 리플렉션 기반 프로퍼티 UI 흐름(`UPropertyRenderer`, `UTargetActorTransformWidget`)을 그대로 활용하고, 최소한의 특수 케이스만 추가한다.

## 관련 코드 위치
- 디테일 패널 엔트리 포인트: `Mundi/Source/Slate/Widgets/TargetActorTransformWidget.cpp`
  - `RenderSelectedComponentDetails()`에서 선택된 컴포넌트의 프로퍼티를 그림.
- 리플렉션 UI: `Mundi/Source/Slate/Widgets/PropertyRenderer.{h,cpp}`
  - `RenderProperty()`와 타입별 렌더러에서 드롭다운/버튼을 구현.
  - 리소스 캐싱(`CacheResources()`) 로직 참고.
- 파티클 실행 로직: `Mundi/Source/Runtime/Engine/Particles/ParticleSystemComponent.{h,cpp}`
  - `Template`, `TickComponent()`, `UpdateParticles()`, `ResetInstances()`, `Restart()` 등이 핵심.

## 구현 절차
1) **컴포넌트 데이터 확장**
- `UParticleSystemComponent`에 커스텀 재생 속도 프로퍼티를 추가.
  ```cpp
  UPROPERTY(EditAnywhere, Category="ParticleSystem")
  float CustomPlaybackRate = 1.0f;
  ```
- `TickComponent()`와 `UpdateParticles()`에서 전달받은 `DeltaTime`에 `CustomPlaybackRate`를 곱해 사용하도록 수정.
  - `SystemTime` 증가 및 `UpdateEmitterInstance()` 호출 전에 `float ScaledDelta = DeltaTime * FMath::Max(CustomPlaybackRate, 0.0f);` 형태로 적용.
  - 0보다 작은 값은 방어적으로 0으로 클램프해 역재생 오동작을 막는다.

2) **Reset 버튼 동작 정의**
- 버튼 클릭 시 수행할 공용 함수로 `Restart()`를 그대로 사용하거나, 필요 시 `CustomPlaybackRate`를 1.0으로 리셋하는 래퍼 함수를 추가.
  ```cpp
  void ResetToDefaultState()
  {
      CustomPlaybackRate = 1.0f;
      Restart(); // Stop + ResetInstances + Activate
  }
  ```

3) **디테일 패널 진입점에 커스텀 UI 훅 추가**
- `RenderSelectedComponentDetails()`에서 `UParticleSystemComponent`인지 검사 후, 공용 프로퍼티 렌더링 전에 전용 위젯을 한 덩어리로 그림.
  ```cpp
  if (auto* PSC = Cast<UParticleSystemComponent>(SelectedComponent))
  {
      RenderParticleSystemControls(*PSC); // 새 헬퍼 함수
      ImGui::Separator();
  }
  UPropertyRenderer::RenderAllPropertiesWithInheritance(SelectedComponent);
  ```
- 이 헬퍼 함수 내부에서 드롭다운/버튼/슬라이더를 배치한다. 기존 패널 레이아웃은 `TargetActorTransformWidget`에서 사용하던 `ImGui::Text/Separator` 스타일을 맞춘다.

4) **Template 드롭다운 구현**
- 리소스 캐시:
  - `UPropertyRenderer::CacheResources()`에 `CachedParticleSystemPaths`/`CachedParticleSystemItems`(파일명만) 배열을 추가하고, `UResourceManager::GetAllFilePaths<UParticleSystem>()`로 채운다.
  - `"None"` 항목을 0번에 넣어 null 선택을 가능하게 한다.
- 렌더러:
  - `RenderProperty()` 분기에서 `EPropertyType::ObjectPtr`이지만 `Prop.Metadata["TypeName"] == "UParticleSystem"`인 경우를 특수 처리하거나, 새 `EPropertyType::ParticleSystem`을 추가해 전용 렌더러(`RenderParticleSystemProperty`)를 만든다.
  - 콤보 박스 선택 시:
    - 선택된 인덱스로 새 경로를 얻어 `UResourceManager::Load<UParticleSystem>(Path)` 호출.
    - `UParticleSystemComponent::SetTemplate(NewAsset)` 호출로 인스턴스 생성/정리를 위임.
    - 변경 시 `Restart()`로 즉시 재생 상태를 초기화하면 에디터에서 결과를 바로 확인 가능.

5) **Reset 버튼 구현**
- `RenderParticleSystemControls()`에서 `ImGui::Button("Reset")` 사용.
- 클릭 시 `PSC.ResetToDefaultState()` (또는 `CustomPlaybackRate = 1.0f; PSC.Restart();`)를 호출해 러닝 상태와 배율을 모두 초기화.

6) **Custom Playback Rate 입력**
- `ImGui::DragFloat("Custom Playback Rate", &PSC.CustomPlaybackRate, 0.01f, 0.0f, 10.0f, "%.2f")` 형태로 배치.
- 값 변경 시 음수 클램프와 상한(예: 10배) 정도만 두어 폭주를 방지한다.
- 실제 틱에서는 위 1) 단계에서 만든 `ScaledDelta`를 사용하므로 별도 Setter 없이도 동작한다.

7) **기존 리플렉션 UI와의 정합성**
- `UPropertyRenderer::RenderAllPropertiesWithInheritance()`는 계속 호출하되, Template 프로퍼티가 새 렌더러로 교체되므로 중복 표시를 피하려면 Template에 `"HideInDetails"` 메타데이터를 주거나, 커스텀 위젯에서 처리할 때만 `return`하여 기본 렌더를 스킵한다.
- 코드 생성기(`BuildTools/CodeGenerator`)를 사용하는 경우, 새 `EPropertyType` 추가 시 생성 스크립트와 매핑 테이블(`Generated` 외부)도 함께 수정해야 한다.

## Tick 속도 조절 로직 조사
- **월드 전역 배율**: `Mundi/Source/Runtime/Engine/GameFramework/World.cpp`에서 `RequestSlomo`/`RequestHitStop`으로 설정된 `TimeDilation`과 `TimeStopDilation`을 적용해 `GameDelta`를 계산 후 액터 틱에 사용.
- **액터별 배율**: 동일 파일의 `UWorld::Tick()`에서 `Actor->Tick(GetDeltaTime(Game) * Actor->GetCustomTimeDillation())`로 개별 스케일을 곱함. `ActorTimingMap`을 통해 지속시간 만료 시 자동 복구.
- **액터 API**: `Mundi/Source/Runtime/Core/Object/Actor.{h,cpp}`의 `SetCustomTimeDillation()`이 `CustomTimeDillation`을 갱신하고 월드 맵에 남은 Duration을 기록. 기본값은 1.0.
- **스크립트 연동**: `Mundi/Source/Runtime/Engine/Scripting/LuaManager.cpp`에서 `SetSlomo`, `HitStop`, `TargetHitStop`을 제공해 Lua에서 월드/타깃 액터의 틱 속도를 제어.
- **컴포넌트 수준**: 현재는 상위 액터의 DeltaTime만 따르며 별도 배율 없음. 위 섹션의 `CustomPlaybackRate`를 추가하면 파티클 컴포넌트에서만 세밀한 재생 속도 제어가 가능해진다.

## 마무리 체크리스트
- 새 리소스 캐시/렌더러 추가 후 `UPropertyRenderer::ClearResourcesCache()` 호출 경로가 있다면 필요 시 갱신.
- 파티클 에셋 로딩이 실패할 때를 대비해 `None` 선택 시 `Template=nullptr`, `EmitterInstances` 정리, `bIsActive=false` 처리 확인.
- 기능 추가 후 코드젠/빌드: `GenerateBindings.bat`(또는 `BuildTools/Python/python.exe BuildTools/CodeGenerator/generate.py ...`) → `msbuild Mundi.sln /p:Configuration=Debug /p:Platform=x64`.
