#pragma once
#include "Object.h"
#include "ParticleModule.h"

class UParticleSystem;
class UParticleEmitter;
class UParticleLODLevel;
class SCascadeCurveEditor;

// Emitter editor render mode (how particles are displayed in the editor viewport)
enum class EEmitterEditorRenderMode : uint8
{
    Normal = 0,    // Normal rendering
    Points,        // Render as points
    Cross,         // Render as crosses
    None           // Don't render (but still simulate)
};

// Per-emitter editor state (not saved with asset, only used in editor)
struct FEmitterEditorState
{
    bool bIsVisible = true;                                    // V toggle - visibility in viewport
    EEmitterEditorRenderMode RenderMode = EEmitterEditorRenderMode::Normal;  // R toggle - render mode
    bool bIsSolo = false;                                      // S toggle - solo mode
};

class SCascadeEmittersPanel
{
public:
    SCascadeEmittersPanel() = default;
    ~SCascadeEmittersPanel() = default;

    void SetEditingSystem(UParticleSystem* InSystem);
    UParticleSystem* GetEditingSystem() const { return EditingSystem; }

    // Draw the panel content inside an allocated region
    void Render(float width, float height);

    int32 GetSelectedEmitterIndex() const { return SelectedEmitterIndex; }
    void SetSelectedEmitterIndex(int32 Index) { SelectedEmitterIndex = Index; }

    UParticleModule* GetSelectedModule() const { return SelectedModule; }
    void SetSelectedModule(UParticleModule* Module) { SelectedModule = Module; }

    // Set curve editor reference (for input coordination)
    void SetCurveEditor(SCascadeCurveEditor* InCurveEditor) { CurveEditor = InCurveEditor; }

    // Editor state accessors
    FEmitterEditorState& GetEmitterEditorState(int32 EmitterIndex);
    bool IsEmitterVisibleInEditor(int32 EmitterIndex) const;
    bool HasAnySoloEmitter() const;

private:
    void EnsureEditingSystem();
    void EnsureEditorStates();  // Ensure editor states array matches emitter count
    UParticleEmitter* CreateDefaultSpriteEmitter();
    UParticleEmitter* CreateDefaultMeshEmitter();
    UParticleEmitter* CreateDefaultBeamEmitter();
    UParticleEmitter* CreateDefaultRibbonEmitter();

    // UI Rendering helpers
    void RenderModuleCard(UParticleModule* module, UParticleLODLevel* parentLOD, int32 moduleIndex, const char* moduleName, const ImVec4& backgroundColor, float width, float height, bool showCheckbox);
    ImVec4 GetModuleColor(const FString& moduleName);

private:
    UParticleSystem* EditingSystem = nullptr; // temporary, in-memory
    int32 SelectedEmitterIndex = -1;
    UParticleModule* SelectedModule = nullptr; // Currently selected module for details panel

    // Per-emitter editor state (visibility, render mode, solo)
    TArray<FEmitterEditorState> EmitterEditorStates;

    // Drag-and-drop state
    int32 DraggedEmitterIndex = -1;
    UParticleModule* DraggedModule = nullptr;

    // Frame-local click tracking (reset each frame in Render)
    bool bClickedOnItemThisFrame = false;

    // Panel icon
    class UTexture* IconPanelEmitters = nullptr;

    // Reference to curve editor for input coordination
    SCascadeCurveEditor* CurveEditor = nullptr;
};

