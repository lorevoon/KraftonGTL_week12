#pragma once
#include "ParticleModule.h"
#include "SViewerWindow.h"

class UEditorAssetPreviewContext;
class UTexture;
class SCascadeEmittersPanel;

// Minimal Cascade-style particle editor shell
// Provides: top toolbar, left column (viewport + details), right column (emitters + curves)
// Viewport renders via ImGui::Image using the viewer state's FViewport
class SParticleSystemEditorWindow : public SViewerWindow
{
public:
    SParticleSystemEditorWindow();
    ~SParticleSystemEditorWindow() override;

    // SWindow override
    void OnRender() override;

protected:
    // SViewerWindow requirements
    ViewerState* CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context) override;
    void DestroyViewerState(ViewerState*& State) override;
    FString GetWindowTitle() const override { return "Cascade Particle Editor"; }

    void PreRenderViewportUpdate() override;

private:
    // Layout state
    float ColumnSplitRatio = 0.4f;      // Left vs Right (smaller = narrower left column for square viewport)
    float LeftRowSplitRatio = 0.6f;     // Viewport vs Details (left column)
    float RightRowSplitRatio = 0.45f;   // Emitters vs Curves (right column)

    void RenderLeftColumn(float width, float height);
    void RenderRightColumn(float width, float height);
    void RenderViewportArea(float width, float height);
    void RenderDetailsPanel(float width, float height);
    void RenderProperty(UParticleModule* Module, const struct FProperty* Prop);
    void RenderSystemProperty(class UParticleSystem* System, const struct FProperty* Prop);
    void RenderEmitterProperty(class UParticleEmitter* Emitter, const struct FProperty* Prop);

    // Toolbar icons and helpers
    void LoadToolbarIcons();
    UTexture* IconNew = nullptr;
    UTexture* IconSave = nullptr;
    UTexture* IconLoad = nullptr;

    // Particle Editor specific icons
    UTexture* IconRestart = nullptr;
    UTexture* IconBackgroundColor = nullptr;
    UTexture* IconBounds = nullptr;
    UTexture* IconOriginAxis = nullptr;
    UTexture* IconParticle = nullptr;
    UTexture* IconLODFirst = nullptr;
    UTexture* IconLODPrev = nullptr;
    UTexture* IconLODNext = nullptr;
    UTexture* IconLODLast = nullptr;
    UTexture* IconLODInsertBefore = nullptr;
    UTexture* IconLODInsertAfter = nullptr;
    UTexture* IconLODDelete = nullptr;

    // Panel icons
    UTexture* IconPanelDetails = nullptr;
    UTexture* IconPanelCurves = nullptr;

    float IconSize = 24.0f;

    // Panels
    SCascadeEmittersPanel* EmittersPanel = nullptr;

    // Viewport menu state
    enum class EDetailMode { Low, Medium, High };
    EDetailMode CurrentDetailMode = EDetailMode::High;

    // Time menu state
    bool bIsPlaying = true;
    bool bRealtime = true;
    bool bLoopSimulation = true;
    float AnimSpeed = 1.0f;

    // Helper to get preview component
    class UParticleSystemComponent* GetPreviewComponent() const;
    // Apply current time settings to the preview component
    void ApplyTimeSettings();
    // Sync emitter editor states (visibility, render mode, solo) to emitter instances
    void SyncEmitterEditorStates(class UParticleSystemComponent* PreviewComp);

    // Deferred command pattern for file operations
    enum class EParticleEditorCommand { None, New, Load, Save };
    EParticleEditorCommand PendingCommand = EParticleEditorCommand::None;
    void ProcessPendingCommands();
    void OnNewParticleSystem();
    void OnLoadParticleSystem();
    void OnSaveParticleSystem();

    // Scene에서 Cascade Editor를 연 소스 컴포넌트 (있는 경우)
    // 다른 이름으로 저장 시 이 컴포넌트의 Template도 업데이트해야 함
    class UParticleSystemComponent* SourceSceneComponent = nullptr;
};
