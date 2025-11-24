#pragma once
#include "Object.h"
#include "ParticleModule.h"

class UParticleSystem;
class UParticleEmitter;
class UParticleLODLevel;

class SCascadeEmittersPanel
{
public:
    SCascadeEmittersPanel() = default;
    ~SCascadeEmittersPanel() = default;

    void SetEditingSystem(UParticleSystem* InSystem) { EditingSystem = InSystem; }
    UParticleSystem* GetEditingSystem() const { return EditingSystem; }

    // Draw the panel content inside an allocated region
    void Render(float width, float height);

    int32 GetSelectedEmitterIndex() const { return SelectedEmitterIndex; }
    void SetSelectedEmitterIndex(int32 Index) { SelectedEmitterIndex = Index; }

    UParticleModule* GetSelectedModule() const { return SelectedModule; }
    void SetSelectedModule(UParticleModule* Module) { SelectedModule = Module; }

private:
    void EnsureEditingSystem();
    UParticleEmitter* CreateDefaultSpriteEmitter();

    // UI Rendering helpers
    void RenderModuleCard(UParticleModule* module, const char* moduleName, const ImVec4& backgroundColor, float width, float height, bool showCheckbox);
    ImVec4 GetModuleColor(const FString& moduleName);

private:
    UParticleSystem* EditingSystem = nullptr; // temporary, in-memory
    int32 SelectedEmitterIndex = -1;
    UParticleModule* SelectedModule = nullptr; // Currently selected module for details panel
};

