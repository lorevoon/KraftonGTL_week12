# ParticleSystem Template ComboBox 구현 계획

## 개요

UParticleSystemComponent의 Template 프로퍼티를 Property Window에서 ComboBox로 선택할 수 있도록 구현합니다.
StaticMesh/SkeletalMesh와 동일한 패턴으로 구현합니다.

## 작업 목록

### Task 1: ResourceManager에 SetParticleSystems() 추가

**파일:** `Mundi/Source/Runtime/AssetManagement/ResourceManager.h`

public 영역에 선언 추가:
```cpp
void SetParticleSystems();
```

protected 영역에 멤버 변수 추가:
```cpp
TArray<UParticleSystem*> ParticleSystems;
```

**파일:** `Mundi/Source/Runtime/AssetManagement/ResourceManager.cpp`

SetAnimations() 함수 근처에 구현 추가:
```cpp
void UResourceManager::SetParticleSystems()
{
    ParticleSystems = GetAll<UParticleSystem>();
}
```

---

### Task 2: FParticleLoader 클래스 생성

**파일:** `Mundi/Source/Editor/ParticleLoader.h` (신규)

FObjManager와 동일한 패턴으로 static 함수만 사용 (싱글톤 인스턴스 없음):

```cpp
#pragma once
#include "UEContainer.h"

class FParticleLoader
{
public:
    // Data/ParticleSystems/ 디렉토리의 .json 파일을 ResourceManager에 preload
    static void Preload();
};
```

**파일:** `Mundi/Source/Editor/ParticleLoader.cpp` (신규)

```cpp
#include "pch.h"
#include "ParticleLoader.h"
#include "ResourceManager.h"
#include "ParticleSystem.h"
#include "GlobalConsole.h"
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

void FParticleLoader::Preload()
{
    fs::path DataDir = fs::current_path() / "Data" / "ParticleSystems";

    if (!fs::exists(DataDir) || !fs::is_directory(DataDir))
    {
        UE_LOG("FParticleLoader::Preload: Directory not found: %s",
               WideToUTF8(DataDir.wstring()).c_str());
        return;
    }

    size_t LoadedCount = 0;
    std::unordered_set<FString> ProcessedFiles;

    for (const auto& Entry : fs::recursive_directory_iterator(DataDir))
    {
        if (!Entry.is_regular_file())
            continue;

        const fs::path& Path = Entry.path();
        FString Extension = WideToUTF8(Path.extension().wstring());
        std::transform(Extension.begin(), Extension.end(), Extension.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (Extension == ".json")
        {
            FString PathStr = NormalizePath(WideToUTF8(Path.wstring()));

            if (ProcessedFiles.find(PathStr) == ProcessedFiles.end())
            {
                ProcessedFiles.insert(PathStr);
                UResourceManager::GetInstance().Load<UParticleSystem>(PathStr);
                ++LoadedCount;
            }
        }
    }

    RESOURCE.SetParticleSystems();

    UE_LOG("FParticleLoader::Preload: Loaded %zu particle systems from %s",
           LoadedCount, WideToUTF8(DataDir.wstring()).c_str());
}
```

---

### Task 3: Engine에서 FParticleLoader::Preload() 호출

**파일:** `Mundi/Source/Runtime/Engine/GameFramework/EditorEngine.cpp`

include 추가:
```cpp
#include "ParticleLoader.h"
```

FObjManager::Preload() 호출 후 (line 195 근처)에 추가:
```cpp
FParticleLoader::Preload();
```

**파일:** `Mundi/Source/Runtime/Engine/GameFramework/GameEngine.cpp`

include 추가:
```cpp
#include "ParticleLoader.h"
```

FObjManager::Preload() 호출 후 (line 197 근처)에 추가:
```cpp
FParticleLoader::Preload();
```

---

### Task 4: EPropertyType::ParticleSystem 추가

**파일:** `Mundi/Source/Runtime/Core/Object/Property.h`

enum class EPropertyType에서 Count 앞에 추가:
```cpp
enum class EPropertyType : uint8
{
    // ... existing types ...
    Curve,
    ParticleSystem,  // ← 추가
    Count
};
```

---

### Task 5: ADD_PROPERTY_PARTICLESYSTEM 매크로 추가

**파일:** `Mundi/Source/Runtime/Core/Object/ObjectMacros.h`

ADD_PROPERTY_AUDIO 매크로 근처에 추가:
```cpp
#define ADD_PROPERTY_PARTICLESYSTEM(VarType, VarName, CategoryName, bEditAnywhere, ...) \
    { \
        static_assert(std::is_array_v<std::remove_reference_t<decltype(CategoryName)>>, \
                      "CategoryName must be a string literal!"); \
        FProperty Prop; \
        Prop.Name = #VarName; \
        Prop.Type = EPropertyType::ParticleSystem; \
        Prop.Offset = offsetof(ThisClass_t, VarName); \
        Prop.Category = CategoryName; \
        Prop.bIsEditAnywhere = bEditAnywhere; \
        Prop.Tooltip = "" __VA_ARGS__; \
        Class->AddProperty(Prop); \
    }
```

---

### Task 6: PropertyRenderer에 ParticleSystem 렌더링 추가

**파일:** `Mundi/Source/Slate/Widgets/PropertyRenderer.h`

private 함수 선언 추가 (RenderMaterialProperty 근처):
```cpp
static bool RenderParticleSystemProperty(const FProperty& Prop, void* Instance);
```

private 캐시 변수 추가 (CachedScriptItems 근처):
```cpp
static TArray<FString> CachedParticleSystemPaths;
static TArray<FString> CachedParticleSystemItems;
```

**파일:** `Mundi/Source/Slate/Widgets/PropertyRenderer.cpp`

**A. 파일 상단에 캐시 변수 정의 추가:**
```cpp
TArray<FString> UPropertyRenderer::CachedParticleSystemPaths;
TArray<FString> UPropertyRenderer::CachedParticleSystemItems;
```

**B. CacheResources() 함수에 ParticleSystem 캐싱 추가 (Sound 캐싱 코드 뒤):**
```cpp
// 6. ParticleSystem
if (CachedParticleSystemPaths.IsEmpty() && CachedParticleSystemItems.IsEmpty())
{
    CachedParticleSystemPaths = ResMgr.GetAllFilePaths<UParticleSystem>();
    for (const FString& path : CachedParticleSystemPaths)
    {
        std::filesystem::path fsPath(UTF8ToWide(path));
        CachedParticleSystemItems.push_back(WideToUTF8(fsPath.filename().wstring()));
    }
    CachedParticleSystemPaths.Insert("", 0);
    CachedParticleSystemItems.Insert("None", 0);
}
```

**C. ClearResourcesCache() 함수에 추가:**
```cpp
CachedParticleSystemPaths.Empty();
CachedParticleSystemItems.Empty();
```

**D. RenderProperty() 함수의 switch문에 case 추가:**
```cpp
case EPropertyType::ParticleSystem:
    bChanged = RenderParticleSystemProperty(Property, ObjectInstance);
    break;
```

**E. RenderParticleSystemProperty() 함수 구현 추가:**
```cpp
bool UPropertyRenderer::RenderParticleSystemProperty(const FProperty& Prop, void* Instance)
{
    UParticleSystem** PSPtr = Prop.GetValuePtr<UParticleSystem*>(Instance);

    FString CurrentPath;
    if (*PSPtr)
    {
        CurrentPath = (*PSPtr)->GetFilePath();
    }

    if (CachedParticleSystemPaths.empty())
    {
        ImGui::Text("%s: <No Particle Systems>", Prop.Name);
        return false;
    }

    int SelectedIdx = -1;
    for (int i = 0; i < static_cast<int>(CachedParticleSystemPaths.size()); ++i)
    {
        if (CachedParticleSystemPaths[i] == CurrentPath)
        {
            SelectedIdx = i;
            break;
        }
    }

    TArray<const char*> ItemsPtr;
    ItemsPtr.reserve(CachedParticleSystemItems.size());
    for (const FString& item : CachedParticleSystemItems)
    {
        ItemsPtr.push_back(item.c_str());
    }

    ImGui::SetNextItemWidth(240);
    if (ImGui::Combo(Prop.Name, &SelectedIdx, ItemsPtr.data(), static_cast<int>(ItemsPtr.size())))
    {
        if (SelectedIdx >= 0 && SelectedIdx < static_cast<int>(CachedParticleSystemPaths.size()))
        {
            UObject* Object = static_cast<UObject*>(Instance);
            if (UParticleSystemComponent* PSComp = Cast<UParticleSystemComponent>(Object))
            {
                PSComp->SetTemplate(
                    UResourceManager::GetInstance().Load<UParticleSystem>(CachedParticleSystemPaths[SelectedIdx])
                );
            }
            else
            {
                *PSPtr = UResourceManager::GetInstance().Load<UParticleSystem>(CachedParticleSystemPaths[SelectedIdx]);
            }
            return true;
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(CurrentPath.c_str());
        ImGui::EndTooltip();
    }

    return false;
}
```

---

### Task 7: Generated 파일 수정

**파일:** `Mundi/Generated/UParticleSystemComponent.generated.cpp`

line 41 수정:

Before:
```cpp
ADD_PROPERTY(UParticleSystem*, Template, "ParticleSystem", true)
```

After:
```cpp
ADD_PROPERTY_PARTICLESYSTEM(UParticleSystem*, Template, "ParticleSystem", true)
```

---

## 수정 파일 요약

| 파일 | 작업 |
|------|------|
| `Mundi/Source/Editor/ParticleLoader.h` | **신규** |
| `Mundi/Source/Editor/ParticleLoader.cpp` | **신규** |
| `Mundi/Source/Runtime/Engine/GameFramework/EditorEngine.cpp` | 수정 |
| `Mundi/Source/Runtime/Engine/GameFramework/GameEngine.cpp` | 수정 |
| `Mundi/Source/Runtime/AssetManagement/ResourceManager.h` | 수정 |
| `Mundi/Source/Runtime/AssetManagement/ResourceManager.cpp` | 수정 |
| `Mundi/Source/Runtime/Core/Object/Property.h` | 수정 |
| `Mundi/Source/Runtime/Core/Object/ObjectMacros.h` | 수정 |
| `Mundi/Source/Slate/Widgets/PropertyRenderer.h` | 수정 |
| `Mundi/Source/Slate/Widgets/PropertyRenderer.cpp` | 수정 |
| `Mundi/Generated/UParticleSystemComponent.generated.cpp` | 수정 |

## 예상 결과

1. 에디터 시작 시 `Data/ParticleSystems/*.json` 파일이 자동으로 ResourceManager에 로드됨
2. ParticleSystemComponent의 Template 프로퍼티가 ComboBox로 표시됨
3. ComboBox에서 로드된 ParticleSystem 목록 선택 가능
4. 선택 시 해당 ParticleSystem이 컴포넌트의 Template으로 설정됨
